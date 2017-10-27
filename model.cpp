#include <dirent.h>
#include <iostream>
#include <essentia/pool.h>
#include <essentia/algorithmfactory.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <math.h>
#include <string>
#include <algorithm>
#include <sys/resource.h>
#include <thread>
#include <ctime>
#include <time.h>
#include <chrono>
#include <fstream>
#include <mutex>          // std::mutex
#include <unordered_map>
#include <unordered_set>

using namespace std;
using namespace essentia;
using namespace essentia::standard;
using namespace chrono;

const int N = 16000;
const int CLUSTER_K = (1<<16);
const int MAX_M = 25, MAX_K = CLUSTER_K;
float cen[MAX_K][12*MAX_M];
int BIT, M, K;

struct Data{
    Real version_id;
    string name;
    vector<Real> glo_hpcp;
    vector<vector<Real> > hpcp;
    vector<int> seq;
};
struct Data data[N];
struct Node{
    int song_id;
    int v;
};
bool cmp(Node a, Node b){
    return a.v > b.v;
}

vector<Node> *hash_tree;
Node hit_cnt[N][N];
int hash_cnt[N];

void calc_ftif(int cnt){
    for(int i=0; i<(1<<BIT); i++)
        for(int j=0; j<(1<<BIT); j++){
            int key = (i<<BIT)+j;
            for(int k=0; k<hash_tree[key].size(); k++)
                //hash_tree[key][k].v = 0;
                hash_tree[key][k].v *= log((float)cnt / hash_tree[key].size()) / hash_cnt[i];
        }
}
void generate_hash_tree(int skipping, int cnt){
    
    for(int i=0; i<cnt; i++){
        for(int j=0; j<data[i].seq.size(); j++)
            for(int k=j+1; k<min(int(data[i].seq.size()), j+skipping); k++){
                if(data[i].seq[j] == data[i].seq[k])
                    continue;
                long long key = ((long long)data[i].seq[j]<<BIT)+data[i].seq[k];
                if(hash_tree[key].empty() || hash_tree[key].back().song_id != i)
                    hash_tree[key].push_back((Node){i, 1});
                else
                    hash_tree[key].back().v += 1;
                hash_cnt[i]++;
            }
    }
}

float get_seq(vector<vector<Real> > hpcp, vector<int> &seq){
    // printf("%d ", M);
    float error = 0;
    for(int i=M-1; i<hpcp.size(); i++){
        float _min = 12.0*M*10000000;
        seq.push_back(0);
        for(int k=0; k<K; k++){
            int fla_d = 0;
            float dis = 0;
            for(int j=i-(M-1); j<=i; j++)
                for(int d=0; d<12; d++){
                    float tmp = hpcp[j][d] - cen[k][fla_d++];
                    dis += tmp*tmp;
                }
            if(dis < _min){
                _min = dis, seq[i-(M-1)] = k;
            }
       }
       error += _min / 1000000;
    }
    return error / (hpcp.size() - M);
}

void generate_base_seq(int start, int end, float *error){
    *error = 0;
    for(int k=start; k<=end; k++){
        vector<int> seq;
        vector<vector<Real> > tran_hpcp = data[k].hpcp;
        for(int i=0; i<tran_hpcp.size(); i++)
            for(int j=0; j<12; j++)
                tran_hpcp[i][j] = data[k].hpcp[i][j] * 1000;
        *error += get_seq(tran_hpcp, seq);
        data[k].seq = seq;
    }
}

double search_hash_tree(int skipping, int transposition, int song_id, int cnt){
    int *hit_tmp_cnt = new int[cnt];
    for(int i=0; i<cnt; i++)
        hit_cnt[song_id][i].song_id = i;

    vector<vector<Real> > tran_hpcp = data[song_id].hpcp;
    for(int key=-transposition; key<=transposition; key++){
        memset(hit_tmp_cnt, 0, sizeof(int)*cnt);
        vector<int> seq;
        unordered_map<long long, int> hash_pattern;
        unordered_set<int> filter_songs;
        for(int i=0; i<tran_hpcp.size(); i++)
            for(int j=0; j<12; j++)
                tran_hpcp[i][j] = data[song_id].hpcp[i][(j+key+12)%12] * 1000;
        get_seq(tran_hpcp, seq);
        for(int j=0; j<seq.size(); j++){
            for(int k=j+1; k<min(int(seq.size()), j+skipping); k++){
                if(seq[j] == seq[k])
                    continue;
                long long key = ((long long)seq[j]<<BIT)+seq[k];
                if(hash_pattern.find(key) == hash_pattern.end())
                    hash_pattern[key] = 1;
                else
                    hash_pattern[key] ++;
            } 
        }
        for(auto iter=hash_pattern.begin(); iter!=hash_pattern.end(); iter++){
            long long hash_num = iter->first;
            int v = iter->second;
            for(int j=0; j<hash_tree[hash_num].size(); j++){
                hit_tmp_cnt[hash_tree[hash_num][j].song_id] += min(hash_tree[hash_num][j].v, v);
                filter_songs.insert(hash_tree[hash_num][j].song_id);
            }
        }
        for(auto iter=filter_songs.begin(); iter!=filter_songs.end(); iter++)
            hit_cnt[song_id][*iter].v = max(hit_tmp_cnt[*iter], hit_cnt[song_id][*iter].v);
    }
    sort(&hit_cnt[song_id][0], &hit_cnt[song_id][cnt], cmp);
    if(song_id < cnt)
        hit_cnt[song_id][song_id].v = 0x1fffffff;
    delete [] hit_tmp_cnt; 
}

void work(int st, int fi, int skipping, int transposition, int cnt){
    for(; st<=fi; st++){
        search_hash_tree(skipping, transposition, st, cnt);
    }
}

void result_ref_que(int ref_cnt, int cnt, FILE *fp){
    float total_rate = 0, mean_rank = 0, mean_top10 = 0;
    for(int song_id=ref_cnt; song_id<cnt; song_id++){
        int right_count = 0;
        float right_rate = 0;
        fprintf(fp, "%d ", song_id);
        for(int i=0; i<ref_cnt; i++){
            fprintf(fp, "%d ", hit_cnt[song_id][i].song_id);
            int v_id = hit_cnt[song_id][i].song_id;
            if(abs(data[song_id].version_id - data[v_id].version_id) < 0.1){
                if(!right_count)
                    mean_rank += (i+1);
                if(i<10)
                    mean_top10 += 1;
                right_count++;
                right_rate += (float)right_count/(i+1);
            }
        }
        fprintf(fp, "\n");
        if(!right_count)
            printf("Check if the num. %d has no cover version\n", song_id+1);
        right_rate /= right_count ? right_count : 1;
        total_rate += right_rate;

    }
    mean_rank /= (cnt-ref_cnt), mean_top10 /= ((cnt-ref_cnt) * 10), total_rate /= (cnt-ref_cnt);
    printf("P10:%f MAP:%f MR1:%f ", mean_top10, total_rate, mean_rank);
}

int main(int argc, char* argv[])
{
    //AlgorithmFactory::
    essentia::init();
    AlgorithmFactory& factory = standard::AlgorithmFactory::instance();
    string filename, cen_name;
    string out_dir_name, dir_name, list_name;
    int cnt = 0, ref_cnt = N, process_cnt, skipping, transposition;
    printf("= ="); 
    if(argc == 11){
        out_dir_name = argv[1];
        dir_name = argv[2];
        list_name = argv[3];
        ref_cnt = stoi(argv[4]);
        process_cnt = stoi(argv[5]);
        skipping = stoi(argv[6]);
        transposition = stoi(argv[7]);
        cen_name  = argv[8];
        BIT = stoi(argv[9]);
        M = stoi(argv[10]);
        K = (1 << BIT);
    }else{
        printf("argument error\n");
        return -1;
    }
    ifstream infile(list_name);
    hash_tree = new vector<Node>[((long long)1<<(BIT+BIT))];
    printf("- -");
    while(getline(infile, filename)){
        //cout << dir_name + filename;
        Algorithm* input = AlgorithmFactory::create("YamlInput",
                                        "filename", dir_name + filename);
        Pool pool;
        input->output("pool").set(pool);
        input->compute();
        
        if(cnt < ref_cnt){
            vector<Real> seq = pool.value<vector<Real> >("0");
            for(int j=0; j<seq.size(); j++)
                data[cnt].seq.push_back((int)seq[j]);
        }
     
        vector<Real> version_id = pool.value<vector<Real> >("version_id");
        vector<Real> glo_hpcp = pool.value<vector<Real> >("glo_hpcp");
        vector<vector<Real> > hpcp = pool.value<vector<vector<Real> > >("down_sample_hpcp");
        data[cnt].name = filename, data[cnt].glo_hpcp = glo_hpcp;
        data[cnt].hpcp = hpcp, data[cnt++].version_id = version_id[0];
        pool.clear();

        delete input;
    }
    FILE *cen_fp = fopen(cen_name.data(), "r");
    for(int i=0; i<K; i++)
        for(int j=0; j<12*M; j++){
            fscanf(cen_fp, "%f", &cen[i][j]);
        }
    fclose(cen_fp);
    printf("data read from disk: Done!\n");
    
    clock_t tStart = clock();
    auto tStart1 = high_resolution_clock::now();
    float error = 0;
    thread T[32];
    float errors[32];
    int speed = ref_cnt / process_cnt + (ref_cnt % process_cnt ? 1 : 0), start=0, real_process_cnt=0;
    while(start<ref_cnt){
        T[real_process_cnt] = thread(generate_base_seq, start, start+speed-1<ref_cnt ? start+speed-1 : ref_cnt-1, &errors[real_process_cnt]);
        real_process_cnt++, start += speed;
    }
    for(int i=0; i<real_process_cnt; i++)
        T[i].join();
    while(1){
        int i;
        for(i=0; i<real_process_cnt; i++)
            if(T[i].joinable())
                break;
        if(i == real_process_cnt)
            break;
        this_thread::sleep_for (chrono::seconds(1));
    }
    for(int i=0; i<real_process_cnt; i++)
        error += errors[i];
    generate_hash_tree(skipping, ref_cnt);
    printf("generate hash list: Done!\n");
    
    int tmp = cnt - ref_cnt;
    speed = tmp / process_cnt + (tmp % process_cnt ? 1 : 0), start=ref_cnt, real_process_cnt=0;
    while(start<cnt){
        T[real_process_cnt] = thread(work, start, start+speed-1<cnt ? start+speed-1 : cnt-1, skipping, transposition, ref_cnt);
        real_process_cnt++, start += speed;
    }
    printf("thread:%d speed:%d\n", real_process_cnt, speed, skipping, transposition);
    for(int i=0; i<real_process_cnt; i++)
        T[i].join();
    while(1){
        int i;
        for(i=0; i<real_process_cnt; i++)
            if(T[i].joinable())
                break;
        if(i == real_process_cnt)
            break;
        this_thread::sleep_for (chrono::seconds(1));
    }
    
    FILE *fp = fopen((out_dir_name + string("after_filter")).data(), "w");
    printf("M:%d K:%d skipping:%d transposition:%d ", M, K, skipping, transposition);
    result_ref_que(ref_cnt, cnt, fp);
    duration<double> eplased = high_resolution_clock::now() - tStart1;
    // note that run_time * real_process_cnt = cpu_time_per_query * (cnt - ref_cnt) (the right side is actually smaller)
    printf("cpu_time_per_query:%fs run_time:%fs quantization_error:%f\n", (double)(clock() - tStart) / CLOCKS_PER_SEC / (cnt - ref_cnt), eplased.count(), sqrt(error / ref_cnt));   
    fclose(fp);
    return 0;
}

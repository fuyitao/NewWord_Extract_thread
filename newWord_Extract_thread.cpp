#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <set>
#include <vector>
#include <ctime>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>

using namespace std;

set<string> stopwordSet;  // 停用词典
set<string> wordsSet;     // 分词词典
vector<string> vc_line;                // 存放所有句子
unordered_map<string, int> map_Ngram;  // 1~N元词及词频字典
int freqSum = 0;                       // 总词频
unordered_map<string, vector<int> > map_left_entropyFreq;   // 左熵词频字典
unordered_map<string, vector<int> > map_right_entropyFreq;  // 右熵词频字典
unordered_map<string, double> map_word_MI;                  // 互信息字典
unordered_map<string, double> map_left_entropy;             // 左熵字典
unordered_map<string, double> map_right_entropy;            // 右熵字典
vector<string> vc_WordinNgram;             // 存放Ngram中所有的word
vector<string> vc_WordinLeftEntropyFreq;   // 存放left_entropyFreq中所有的word
vector<string> vc_WordinRightEntropyFreq;  // 存放right_entropyFreq中所有的word

typedef struct ngram_parameter{
    int beg, end;
    int ngram;
    unordered_map<string, int> map_Ngram;
    int freqSum;
}Ngram_Parameter;  // NGram结构体

typedef struct mi_lrentropyfreq_parameter{
    int beg, end;
    int ngram;
    unordered_map<string, vector<int> > map_left_entropyFreq;
    unordered_map<string, vector<int> > map_right_entropyFreq;
    unordered_map<string, double> map_word_MI;
}MI_LREntropyFreq_Parameter;  // 互信息、左右熵词频结构体

typedef struct lrentropy_parameter{
    int beg, end;
    unordered_map<string, double> map_entropy;
}LREntropy_Parameter;  // 左右熵结构体

/*********************  获取字典  *************************/
// 获取停用词典/分词词典
set<string> getWordSet(string wordFile){
    set<string> wordSet;
    ifstream fpin(wordFile.c_str());
    string strline;

    if (!fpin){
	cout << "没有词典文件!" << endl;
    }
    while (getline(fpin, strline)){
	wordSet.insert(strline);
    }
    fpin.close();

    return wordSet;
}

// 将字符串切分成vector
vector<string> Str2Vec(string strline){
    vector<string> vc_str;
    int i = 0;
    while (i < (int)strline.size()){
	if (strline[i] >= 0){
	    vc_str.push_back(strline.substr(i, 1));
	    i += 1;
	}else{
	    vc_str.push_back(strline.substr(i, 3));
	    i += 3;
	}
    }

    return vc_str;
}

// 获取1~N元词及词频
void *getNGramDict(void *dat){
    Ngram_Parameter *para = (Ngram_Parameter *)dat;
    int i, j;
    int freqsum = 0;  // 词频总数
    string strline;
    unordered_map<string, int> map_ngram;
    for (i = para->beg; i < para->end; i++){
	strline = vc_line[i];
	vector<string> vc_str = Str2Vec(strline);  // 存放strline中的每个字符

	// 获取1~N元词
	int beg, end, maxLen;
	for (beg = 0; beg < (int)vc_str.size(); beg++){
	    if (beg+para->ngram > (int)vc_str.size()){
		maxLen = (int)vc_str.size();
	    }else{
		maxLen = beg + para->ngram;
	    }

	    string word = "";
	    for (end = beg; end < maxLen; end++){
		word = word + vc_str[end];
		para->map_Ngram[word] += 1;
		freqsum += 1;
	    }
	}
    }

    para->freqSum = freqsum;

    return 0;
}
/**********************************************************/

/*******************  左右熵和互信息  *********************/
// 计算词内部的互信息—— MI = log(p(x, y) / p(x)*p(y))
double calMIinWord(vector<string> vc_str){
    double min_MI = 1000000;  // 保留词内部中最小的MI
    int i, j, mid;
    string word, wordleft, wordright;  // 计算MI的词、左边的词、右边的词
    word = "";
    double wordProb, wordleftProb, wordrightProb, word_mi;
    for (i = 0; i < (int)vc_str.size(); i++){
	word += vc_str[i];
    }
    wordProb = (double)map_Ngram[word] / freqSum;  // 词的联合概率(p(x, y))

    for (mid = 1; mid <= (int)vc_str.size()-1; mid++){
	wordleft = wordright = "";
	for (i = 0; i < mid; i++){
	    wordleft += vc_str[i];
	}
	for (j = mid; j < (int)vc_str.size(); j++){
	    wordright += vc_str[j];
	}

	wordleftProb = (double)map_Ngram[wordleft] / freqSum;    // 左边词的概率(p(x))
	wordrightProb = (double)map_Ngram[wordright] / freqSum;  // 右边词的概率(p(y))

	word_mi = log(wordProb / (wordleftProb*wordrightProb));

	if (word_mi < min_MI){
	    min_MI = word_mi;
	}
    }

    return min_MI;
}

// 计算左右熵词频和互信息
void *getLREntropyFreq_MI(void *dat){
    MI_LREntropyFreq_Parameter *para = (MI_LREntropyFreq_Parameter *)dat;
    unordered_map<string, int>::iterator it_map;
    string word, wordleft, wordright;
    int freq, freqleft, freqright;
    int wordLen, i, j;
    vector<string>  vc_str;
    double word_mi;
    for (i = para->beg; i < para->end; i++){
	word = vc_WordinNgram[i];
	freq = map_Ngram[word];
	vc_str = Str2Vec(word);  // 切分成单字
	wordLen = vc_str.size();  // 词的长度
	// 2~N元的词计算MI
	if (wordLen >= 2 && wordLen <= para->ngram){
	    word_mi = calMIinWord(vc_str);  // 计算词内部的MI
	    para->map_word_MI[word] = word_mi;
	}

	// 3~N+1元的词获取左右熵的词频
	if (wordLen >= 3 && wordLen <= para->ngram+1){
	    wordleft = wordright = "";  // 左熵词、右熵词
	    for (j = 1; j < (int)vc_str.size(); j++){
		wordleft += vc_str[j];
	    }
	    for (j = 0; j < (int)vc_str.size()-1; j++){
		wordright += vc_str[j];
	    }
	    
	    para->map_left_entropyFreq[wordleft].push_back(freq);    // 存左熵词频
	    para->map_right_entropyFreq[wordright].push_back(freq);  // 存右熵词频
	}
    }

    return 0;
}

// 计算熵—— H(p) = sum((-p(xi))*log(p(xi))) (p(xi)为该词语与其他单字组合的概率)
double cal_Entropy(string word, string local){
    int freq, i;
    double prob;
    int freqsum = 0;          // 熵词频总和
    double entropySum = 0.0;  // 熵值总和
    if (local == "left"){
	for (i = 0; i < map_left_entropyFreq[word].size(); i++){
	    freqsum += map_left_entropyFreq[word][i];
	}
	for (i = 0; i < map_left_entropyFreq[word].size(); i++){
	    prob = (double)map_left_entropyFreq[word][i] / freqsum;  // 概率
	    entropySum += (-prob) * log(prob);                       // 熵值
	}
    }else{
	for (i = 0; i < map_right_entropyFreq[word].size(); i++){
	    freqsum += map_right_entropyFreq[word][i];
	}
	for (i = 0; i < map_right_entropyFreq[word].size(); i++){
	    prob = (double)map_right_entropyFreq[word][i] / freqsum;  // 概率
	    entropySum += (-prob) * log(prob);                        // 熵值
	}
    }

    return entropySum;
}

// 计算左右熵
void *getLeftEntropy(void *dat){
    LREntropy_Parameter *para = (LREntropy_Parameter *)dat;
    string word;
    double entropy;
    int i;
    for (i = para->beg; i != para->end; i++){
	word = vc_WordinLeftEntropyFreq[i];
	entropy = cal_Entropy(word, "left");
	para->map_entropy[word] = entropy;
    }

    return 0;
}
void *getRightEntropy(void *dat){
    LREntropy_Parameter *para = (LREntropy_Parameter *)dat;
    string word;
    double entropy;
    int i;
    for (i = para->beg; i != para->end; i++){
	word = vc_WordinRightEntropyFreq[i];
	entropy = cal_Entropy(word, "right");
	para->map_entropy[word] = entropy;
    }

    return 0;
}

/**********************************************************/

/*********************  创建多线程  ***********************/
// Ngram多线程
void getNGram_thread(string fileName, int ngram, int threadNum){
    // 获取所有句子
    ifstream fpin(fileName.c_str());
    string strline;
    while (getline(fpin, strline)){
	vc_line.push_back(strline);
    }
    fpin.close();
    // 每个thread获取句子的起始和结束位置
    int i, j;
    int interLen = vc_line.size() / threadNum;   // 每个线程中vc_line存储句子的个数
    vector<Ngram_Parameter> vc_para(threadNum);  // 线程vector
    for (i = 0; i < threadNum; i++){
	vc_para[i].ngram = ngram;
	vc_para[i].beg = i * interLen;
	if (i != threadNum-1){
	    vc_para[i].end = (i+1) * interLen;
	}else{
	    vc_para[i].end = vc_line.size();
	}
    }

    // 创建多线程
    vector<pthread_t> pid_list(threadNum);
    for (i = 0; i < threadNum; i++) pthread_create(&(pid_list[i]), NULL, getNGramDict, &vc_para[i]);
    for (i = 0; i < threadNum; i++) pthread_join(pid_list[i], NULL);
    // 多线程数据合并
    unordered_map<string, int>::iterator it_map;
    string word;
    int freq;
    for (i = 0; i < threadNum; i++){
	freqSum += vc_para[i].freqSum;
	for (it_map = vc_para[i].map_Ngram.begin(); it_map != vc_para[i].map_Ngram.end(); it_map++){
	    word = it_map->first;
	    freq = it_map->second;
	    map_Ngram[word] += freq;
	}
    }
    return;
}

// 左右熵词频、互信息多线程
void get_MI_LREntropyFreq_thread(int ngram, int threadNum){
    // 将Ngram中的所有词分配threadNum个结构体中
    int i, j;
    int interLen = map_Ngram.size() / threadNum;  // 每个线程中map_Ngram存储词的个数
    unordered_map<string, int>::iterator it_map;
    vector<MI_LREntropyFreq_Parameter> vc_para(threadNum);  // 线程vector
    string word;
    int freq;
    
    // 存放Ngram中的所有word
    for (it_map = map_Ngram.begin(); it_map != map_Ngram.end(); it_map++){
	vc_WordinNgram.push_back(it_map->first);
    }
    // 每个thread获取word的起始和结束位置
    for (i = 0; i < threadNum; i++){
	vc_para[i].ngram = ngram;
	vc_para[i].beg = i * interLen;
	if (i != threadNum-1){
	    vc_para[i].end = (i+1) * interLen;
	}else{
	    vc_para[i].end = vc_WordinNgram.size();
	}
    }

    // 创建多线程
    vector<pthread_t> pid_list(threadNum);
    for (i = 0; i < threadNum; i++) pthread_create(&(pid_list[i]), NULL, getLREntropyFreq_MI, &vc_para[i]);
    for (i = 0; i < threadNum; i++) pthread_join(pid_list[i], NULL);
    // 多线程数据合并
    unordered_map<string, double>::iterator it_map_MI;
    unordered_map<string, vector<int> >::iterator it_map_Entropy;
    double word_mi;
    for (i = 0; i < threadNum; i++){
	// MI合并
	for (it_map_MI = vc_para[i].map_word_MI.begin(); it_map_MI != vc_para[i].map_word_MI.end(); it_map_MI++){
	    word = it_map_MI->first;
	    word_mi = it_map_MI->second;
	    map_word_MI[word] = word_mi;
	}
	// 左右熵词频合并
	for (it_map_Entropy = vc_para[i].map_left_entropyFreq.begin(); it_map_Entropy != vc_para[i].map_left_entropyFreq.end(); it_map_Entropy++){
	    word = it_map_Entropy->first;
	    for (j = 0; j < (int)it_map_Entropy->second.size(); j++){
		freq = it_map_Entropy->second[j];
		map_left_entropyFreq[word].push_back(freq);
	    }
	}
	for (it_map_Entropy = vc_para[i].map_right_entropyFreq.begin(); it_map_Entropy != vc_para[i].map_right_entropyFreq.end(); it_map_Entropy++){
	    word = it_map_Entropy->first;
	    for (j = 0; j < (int)it_map_Entropy->second.size(); j++){
		freq = it_map_Entropy->second[j];
		map_right_entropyFreq[word].push_back(freq);
	    }
	}
    }

    return;
}

// 计算左右熵
void getLREntropy(){
    unordered_map<string, vector<int> >::iterator it_map;
    double entropy;
    string word;
    // 左熵
    for (it_map = map_left_entropyFreq.begin(); it_map != map_left_entropyFreq.end(); it_map++){
	word = it_map->first;
	entropy = cal_Entropy(word, "left");
	map_left_entropy[word] = entropy;
    }
    // 右熵
    for (it_map = map_right_entropyFreq.begin(); it_map != map_right_entropyFreq.end(); it_map++){
	word = it_map->first;
	entropy = cal_Entropy(word, "right");
	map_right_entropy[word] = entropy;
    }

    return;
}

// 左右熵多线程
void getLREntropy_thread(int threadNum){
    // 将left_entropyFreq和right_entropyFreq中的所有词分配threadNum个结构体中
    int i;
    int interLen_left = map_left_entropyFreq.size() / threadNum;    // 每个线程中left_entropyFreq存储词的个数
    int interLen_right = map_right_entropyFreq.size() / threadNum;  // 每个线程中right_entropyFreq存储词的个数
    vector<LREntropy_Parameter> vc_para_left(threadNum);   // left_entropy线程vector
    vector<LREntropy_Parameter> vc_para_right(threadNum);  // right_entropy线程vector
    unordered_map<string, vector<int> >::iterator it_map;
    string word;
    double entropy;
    
    // 存放LREntropyFreq中的所有word
    for (it_map = map_left_entropyFreq.begin(); it_map != map_left_entropyFreq.end(); it_map++){
	vc_WordinLeftEntropyFreq.push_back(it_map->first);
    }
    for (it_map = map_right_entropyFreq.begin(); it_map != map_right_entropyFreq.end(); it_map++){
	vc_WordinRightEntropyFreq.push_back(it_map->first);
    }
    // 每个thread获取word的起始和结束为止
    for (i = 0; i < threadNum; i++){
	vc_para_left[i].beg = i * interLen_left;
	if (i != threadNum-1){
	    vc_para_left[i].end = (i+1) * interLen_left;
	}else{
	    vc_para_left[i].end = vc_WordinLeftEntropyFreq.size();
	}
    }
    for (i = 0; i < threadNum; i++){
	vc_para_right[i].beg = i * interLen_right;
	if (i != threadNum-1){
	    vc_para_right[i].end = (i+1) * interLen_right;
	}else{
	    vc_para_right[i].end = vc_WordinRightEntropyFreq.size();
	}
    }

    // 创建多线程
    vector<pthread_t> pid_list_left(threadNum);
    vector<pthread_t> pid_list_right(threadNum);
    for (i = 0; i < threadNum; i++) pthread_create(&(pid_list_left[i]), NULL, getLeftEntropy, &vc_para_left[i]);
    for (i = 0; i < threadNum; i++) pthread_create(&(pid_list_right[i]), NULL, getRightEntropy, &vc_para_right[i]);
    for (i = 0; i < threadNum; i++) pthread_join(pid_list_left[i], NULL);
    for (i = 0; i < threadNum; i++) pthread_join(pid_list_right[i], NULL);
    // 多线程合并
    unordered_map<string, double>::iterator it_map_entropy;
    for (i = 0; i < threadNum; i++){
	// 左熵
	for (it_map_entropy = vc_para_left[i].map_entropy.begin(); it_map_entropy != vc_para_left[i].map_entropy.end(); it_map_entropy++){
	    word = it_map_entropy->first;
	    entropy = it_map_entropy->second;
	    map_left_entropy[word] = entropy;
	}
	// 右熵
	for (it_map_entropy = vc_para_right[i].map_entropy.begin(); it_map_entropy != vc_para_right[i].map_entropy.end(); it_map_entropy++){
	    word = it_map_entropy->first;
	    entropy = it_map_entropy->second;
	    map_right_entropy[word] = entropy;
	}
    }

    return;
}

/**********************************************************/

/**********************  输出结果  ************************/
// 输出结果
void OutputResult(string fileName, int threadNum){
    // 阙值
    // 互信息阙值
    double MI_max = 6.0;
    double MI_min = 2.0;
    // 熵阙值
    double Entropy_max = 3.0;
    double Entropy_min = 0.0;
    // 频率阙值
    int Freq_max = 50;
    int Freq_min = 2;
    // 加权平均阙值
    double Score_min = 0.97;
    //
    double MI_val = 0.0;            // 互信息归一化值
    double leftEntropy_val = 0.0;   // 左熵归一化值
    double rightEntropy_val = 0.0;  // 右熵归一化值
    double Freq_val = 0.0;          // 频率归一化值
    double score = 0.0;             // 得分
    //
    stringstream ss;
    ss << threadNum;
    string outName = fileName + "_" + ss.str() + "newwordResult";
    ofstream fpout(outName.c_str());
    unordered_map<string, double>::iterator it_map;
    string word;
    double word_mi, left_entropy, right_entropy;
    int word_freq;
    for (it_map = map_word_MI.begin(); it_map != map_word_MI.end(); it_map++){
	word = it_map->first;
	// 如果word在停用词典/分词词典中,continue
	if (stopwordSet.find(word) != stopwordSet.end() || wordsSet.find(word) != wordsSet.end()){
	    continue;
	}

	// MI,左右熵,freq
	word_mi = map_word_MI[word];
	if (map_left_entropy.find(word) != map_left_entropy.end())
	    left_entropy = map_left_entropy[word];
	else
	    left_entropy = 0.0;
	if (map_right_entropy.find(word) != map_right_entropy.end())
	    right_entropy = map_right_entropy[word];
	else
	    right_entropy = 0.0;
	word_freq = map_Ngram[word];

	// MI_val
	if (word_mi > MI_max)
	    MI_val = 1.0;
	else if (word_mi < MI_min)
	    MI_val = 0.0;
	else
	    MI_val = (word_mi - MI_min) / (MI_max - MI_min);
	// leftEntropy_val
	if (left_entropy > Entropy_max)
	    leftEntropy_val = 1.0;
	else if (left_entropy < Entropy_min)
	    leftEntropy_val = 0.0;
	else
	    leftEntropy_val = (left_entropy - Entropy_min) / (Entropy_max - Entropy_min);
	// rightEntropy_val
	if (right_entropy > Entropy_max)
	    rightEntropy_val = 1.0;
	else if (right_entropy < Entropy_min)
	    rightEntropy_val = 0.0;
	else
	    rightEntropy_val = (right_entropy - Entropy_min) / (Entropy_max - Entropy_min);
	// Freq_val
	if (word_freq > Freq_max)
	    Freq_val = 1;
	else if (word_freq < Freq_min)
	    Freq_val = 0;
	else
	    Freq_val = (double)(word_freq - Freq_min) / (Freq_max - Freq_min);
	//
	score = (MI_val + leftEntropy_val + rightEntropy_val + Freq_val) / 4;

	if (score > Score_min){
	    fpout << word << "\t" << score << "\t" << word_mi << "\t" << left_entropy << "\t" << right_entropy << "\t" << word_freq << endl;
	}
    }

    fpout.close();
}

// 测试输出结果
void output(string fileName, int threadNum){
    stringstream ss;
    ss << threadNum;
    // Ngram
    string outName = fileName + "_" + ss.str() + "_Result";
    unordered_map<string, int>::iterator it_map;
    ofstream fpout(outName.c_str());
    for (it_map = map_Ngram.begin(); it_map != map_Ngram.end(); it_map++){
	fpout << it_map->first << " : " << it_map->second << endl;
    }
    fpout << freqSum << endl;
    fpout.close();
  
    // MI
    string MI_outName = fileName + "_MI_" + ss.str() + "_Result";
    unordered_map<string, double>::iterator it_map_MI;
    ofstream fpout_MI(MI_outName.c_str());
    for (it_map_MI = map_word_MI.begin(); it_map_MI != map_word_MI.end(); it_map_MI++){
	fpout_MI << it_map_MI->first << " : " << it_map_MI->second << endl;
    }
    fpout_MI.close();

    // LREntropyFreq
    string EntropyFreqleft_outName = fileName + "_EntropyFreqleft_" + ss.str() + "_Result";
    string EntropyFreqright_outName = fileName + "_EntropyFreqright_" + ss.str() + "_Result";
    unordered_map<string, vector<int> >::iterator it_map_Entropy;
    ofstream fpout_EntropyFreqleft(EntropyFreqleft_outName.c_str());
    ofstream fpout_EntropyFreqright(EntropyFreqright_outName.c_str());
    for (it_map_Entropy = map_left_entropyFreq.begin(); it_map_Entropy != map_left_entropyFreq.end(); it_map_Entropy++){
	fpout_EntropyFreqleft << it_map_Entropy->first << " : ";
	for (int i = 0; i < (int)it_map_Entropy->second.size(); i++){
	    fpout_EntropyFreqleft << it_map_Entropy->second[i] << " ";
	}
	fpout_EntropyFreqleft << endl;
    }
    for (it_map_Entropy = map_right_entropyFreq.begin(); it_map_Entropy != map_right_entropyFreq.end(); it_map_Entropy++){
	fpout_EntropyFreqright << it_map_Entropy->first << " : ";
	for (int i = 0; i < (int)it_map_Entropy->second.size(); i++){
	    fpout_EntropyFreqright << it_map_Entropy->second[i] << " ";
	}
	fpout_EntropyFreqright << endl;
    }
    fpout_EntropyFreqleft.close();
    fpout_EntropyFreqright.close();

    // LREntropy
    string Entropyleft_outName = fileName + "_Entropyleft_" + ss.str() + "_Result";
    string Entropyright_outName = fileName + "_Entropyright_" + ss.str() + "_Result";
    unordered_map<string, double>::iterator it_map_entropy;
    ofstream fpout_Entropyleft(Entropyleft_outName.c_str());
    ofstream fpout_Entropyright(Entropyright_outName.c_str());
    for (it_map_entropy = map_left_entropy.begin(); it_map_entropy != map_left_entropy.end(); it_map_entropy++){
	fpout_Entropyleft << it_map_entropy->first << " : " << it_map_entropy->second << endl;
    }
    for (it_map_entropy = map_right_entropy.begin(); it_map_entropy != map_right_entropy.end(); it_map_entropy++){
	fpout_Entropyright << it_map_entropy->first << " : " << it_map_entropy->second << endl;
    }
    fpout_Entropyleft.close();
    fpout_Entropyright.close();
}

/************************  main  **************************/
int main(int argc, char *argv[]){
    // 参数
    int ngram = 4;                                  // 默认新词最大字数
    int threadNum = 1;                              // 默认线程数量
    string stopwordFile = "include/stopwords.txt";  // 默认停用词表位置
    string wordsFile = "include/words.txt";         // 默认字典位置
    string inputFile = "";

    // 获取参数
    int i = 1;
    stringstream ss;
    if (argc == 1){
	cout << "option usage: ./main inputfile -s stopword -w words -n ngram -t threadnum" << endl;
	exit(-1);
    }
    string para;
    while (i < argc){
	para = argv[i];
	if (para == "-s"){
	    if (i+1 < argc){
		stopwordFile = argv[i+1];
		i += 2;
	    }else{
		i++;
	    }
	}else if (para == "-w"){
	    if (i+1 < argc){
		wordsFile = argv[i+1];
		i += 2;
	    }else{
		i++;
	    }
	}else if (para == "-n"){
	    if (i+1 < argc){
		ngram = atoi(argv[i+1]);
		i += 2;
	    }else{
		i++;
	    }
	}else if (para == "-t"){
	    if (i+1 < argc){
		threadNum = atoi(argv[i+1]);
		i += 2;
	    }else{
		i++;
	    }
	}else if (inputFile == ""){
	    inputFile = argv[i];
	    i++;
	}else{
	    cout << "Error Input: " << argv[i] << endl;
	    cout << "option usage: ./main inputfile -s stopword -w words -n ngram -t threadnum" << endl;
	    exit(-1);
	}
    }

    // 处理数据
    clock_t start, end;
    stopwordSet = getWordSet(stopwordFile);
    wordsSet = getWordSet(wordsFile);
    cout << "停用词、分词词典加载完毕!" << endl;

    start = clock();  // 开始时间
    getNGram_thread(inputFile, ngram+1, threadNum);  // 创建Ngram多线程
    cout << "Ngram生成完毕!" << endl;
    get_MI_LREntropyFreq_thread(ngram, threadNum);   // 创建word_MI、left_entropyFreq、right_entropyFreq多线程
    cout << "MI_LREntropyFreq生成完毕!" << endl;
//    getLREntropy_thread(threadNum);                  // 创建left_entropy、right_entropy多线程
    getLREntropy();                                  // 计算左右熵
    cout << "LREntroy生成完毕!" << endl;
    OutputResult(inputFile, threadNum);
//    output(inputFile, threadNum);
    end = clock();

    cout << (end-start) / CLOCKS_PER_SEC << "(s)" << endl;

    return 0;
}

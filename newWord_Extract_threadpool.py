#!/usr/bin/env python
#-*- coding: utf-8 -*-

import sys, re, argparse, math, jieba.posseg, threading, threadpool, time
reload(sys)
sys.setdefaultencoding('utf-8')

parser = argparse.ArgumentParser()
parser.add_argument("corpus_file", help="corpus_file")
parser.add_argument("-s", "--stopword", help="StopWord", default="include/stopwords.txt")  # 停用词词典
parser.add_argument("-w", "--words", help="Words.txt", default="include/words.txt")        # 分词词典
parser.add_argument("-n", "--ngram", help="N-gram", default=4, type=int)                   # 需要获取新词的最大字数
parser.add_argument("-t", "--threadnum", help="ThreadNum", default=3, type=int)            # 线程数量

args = parser.parse_args()
filename, stopwordFile, wordsFile, ngram, threadNum = args.corpus_file, args.stopword, args.words, args.ngram, args.threadnum

NgramDict = {}
TagDict = {}
freqSum = 0  # 词频的总和
threadLock = threading.Lock()

##################  获取字典  ####################
# 获取停用词典/分词词典
def getWordDict(wordFile):
    wordDict = set()
    with open(wordFile) as fp:
	for line in fp.readlines():
	    word = line.strip().decode('utf-8')
	    wordDict.add(word)
    return wordDict

# 词与词性组合
def wordAndflag(testline, flaglist, beg, end):
    word_line = ""
    last_flag = ""
    resultline = ""

    for i in range(beg, end+1):
	word = testline[i]
	flag = flaglist[i]
	if last_flag == "":
	    word_line = word
	    last_flag = flag
	else:
	    if flag == last_flag:
		word_line += word
	    else:
		resultline += word_line + "/" + last_flag + " "
		word_line = word
		last_flag = flag
    resultline += word_line + "/" + last_flag

    return resultline

# 获取字典
def getDict(line, n):
    ngramDict = {}
    tagDict = {}
    freqsum = 0

    # tag
    seg = jieba.posseg.cut(line)
    flaglist = []
    for wordflag in seg:
	word = wordflag.word
	flag = wordflag.flag
	for tmpword in word:
	    flaglist.append(flag)
    #
    for beg in range(len(line)):
	maxLen = len(line) if beg+n >= len(line) else beg+n  # 最后一个字的位置
	for end in range(beg, maxLen):
	    word = line[beg: end+1]
	    tag = wordAndflag(line, flaglist, beg, end)

	    # N元词典
	    if word in ngramDict:
		ngramDict[word] += 1
	    else:
		ngramDict[word] = 1
	    # tag词典
	    if word in tagDict:
		if tag in tagDict[word]:
		    tagDict[word][tag] += 1
		else:
		    tagDict[word][tag] = 1
	    else:
		tagDict[word] = {tag: 1}
	    # 词频总和
	    freqsum += 1

    threadLock.acquire()  # 加锁
    # NgramDict
    for word in ngramDict:
	freq = ngramDict[word]
	if word in NgramDict:
	    NgramDict[word] += freq
	else:
	    NgramDict[word] = freq
    # TagDict
    for word in tagDict:
	for tag in tagDict[word]:
	    freq = tagDict[word][tag]
	    if word in TagDict:
		if tag in TagDict[word]:
		    TagDict[word][tag] += freq
		else:
		    TagDict[word][tag] = freq
	    else:
		TagDict[word] = {tag: freq}
    # freqSum
    global freqSum
    freqSum += freqsum
    threadLock.release()  # 释放锁

# 获取1~N元词及词频
def getNGramDict(fileName, n):
    # 获取所有句子
    lineList = []
    fileRead = open(fileName)
    for line in fileRead.readlines():
	testline = line.decode('utf-8').rstrip()
	tmplist = [testline, n]
	lineList.append((tmplist, None))
    fileRead.close()

    # 线程池
    print "创建", threadNum, "个线程"
    pool = threadpool.ThreadPool(threadNum)
    requests = threadpool.makeRequests(getDict, lineList)
    [pool.putRequest(req) for req in requests]
    pool.wait()

##################################################

def test():
    # NgramDict
    for word in NgramDict:
	print word, NgramDict[word]

    # TagDict
    for word in TagDict:
	for tag in TagDict[word]:
	    print word, tag, TagDict[word][tag]

    # freqSum
    print freqSum

def main():
#    stopwordsDict = getWordDict(stopwordFile)  # 停用词词典
#    wordsDict = getWordDict(wordsFile)         # 分词词典
#    print "停用词、分词词典加载完毕!"
    start = time.clock()
    getNGramDict(filename, ngram+1)
    test()
    end = time.clock()
    print "运行时间: %f s" %(end - start)
main()

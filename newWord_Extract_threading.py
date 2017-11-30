#!/usr/bin/env python
#-*- coding: utf-8 -*-

import sys, re, argparse, math, jieba.posseg, threading, time
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

##################################################

###############f###  多线程类  ####################
class Ngram_Thread(threading.Thread):
    def __init__(self, name, lineList, Ngram):
	threading.Thread.__init__(self)
	self.name = name
	self.lineList = lineList
	self.ngram = Ngram
	self.NgramDict = {}
	self.TagDict = {}
	self.freqSum = 0
    def run(self):
	n = self.ngram
	for i in range(len(self.lineList)):
	    testline = self.lineList[i]
	    # tag
	    seg = jieba.posseg.cut(testline)
	    flaglist = []
	    for wordflag in seg:
		word = wordflag.word
		flag = wordflag.flag
		for tmpword in word:
		    flaglist.append(flag)
	    #
	    for beg in range(len(testline)):
		maxLen = len(testline) if beg+n >= len(testline) else beg+n  # 最后一个字的位置
		for end in range(beg, maxLen):
		    word = testline[beg: end+1]
		    tag = wordAndflag(testline, flaglist, beg, end)
		
		    # N元词典
		    if word in self.NgramDict:
			self.NgramDict[word] += 1
		    else:
			self.NgramDict[word] = 1
		    # tag词典
		    if word in self.TagDict:
			if tag in self.TagDict[word]:
			    self.TagDict[word][tag] += 1
			else:
			    self.TagDict[word][tag] = 1
		    else:
			self.TagDict[word] = {tag: 1}

		    self.freqSum += 1  # 词频总和

##################################################

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

# 获取1~N元词及词频
def getNGramDict(fileName, n):
    # 获取所有句子
    lineList = []
    fileRead = open(fileName)
    for line in fileRead.readlines():
	testline = line.decode('utf-8').rstrip()
	lineList.append(testline)
    fileRead.close()

    threads = []
    # 创建新线程
    print "创建", threadNum, "个线程"
    interRate = len(lineList) / threadNum  # 将lineList分成threadNum个组
    for i in range(threadNum):
	if i != threadNum-1:
	    tmpList = lineList[i*interRate: (i+1)*interRate]
	else:
	    tmpList = lineList[i*interRate: ]

	thread = Ngram_Thread("Thread-"+str(i), tmpList, n)
	threads.append(thread)
    
    # 开启新线程
    for i in range(threadNum):
	threads[i].start()

    # 等待所有线程完成
    for i in range(threadNum):
	threads[i].join()

    # 测试线程结果
    global freqSum
    for i in range(threadNum):
	thread = threads[i]
	# NgramDict
	for word in thread.NgramDict:
	    freq = thread.NgramDict[word]
	    if word in NgramDict:
		NgramDict[word] += freq
	    else:
		NgramDict[word] = freq
	# TagDict
	for word in thread.TagDict:
	    for tag in thread.TagDict[word]:
		freq = thread.TagDict[word][tag]
		if word in TagDict:
		    if tag in TagDict[word]:
			TagDict[word][tag] += freq
		    else:
			TagDict[word][tag] = freq
		else:
		    TagDict[word] = {tag: freq}
	# freqSum
#	print thread.name, thread.freqSum
	freqSum += thread.freqSum


##################################################

def test():
    # NgramDict
    for word in NgramDict:
	print word, NgramDict[word]

#    # TagDict
#    for word in TagDict:
#	for tag in TagDict[word]:
#	    print word, tag, TagDict[word][tag]

    # freqSum
#    print freqSum

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

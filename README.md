新词发现的三个不同版本，C++多线程、python多线程、python多线程池（python的多线程是假的，没有提升速度反而因为开启多线程变慢了）。
具体操作：对分好词的所有数据进行计算左右熵、互信息、词频。确定阙值生成新词，最后通过停用词和分词词典进行过滤。

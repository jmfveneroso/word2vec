#!/bin/bash
for filename in docs/*.txt; do
  ./gumbo "$filename" >> local_vector_train_data.txt
done

# dumpindex /mnt/hd0/joao_test/indri_repo/ documenttext $1 > docs/$1.txt
# ./gumbo t


APP_DIR=~/GigaSight/src/indexer/Indexer/pstf/src/
CURRENT_DIR=`pwd`

# echo $APP_DIR
cd $APP_DIR

source ../env/bin/activate
cd $APP_DIR

# echo "python -m pstf.scripts.runSTF msrc21_Lab.pred 6 $PIC_FN"
python -m pstf.scripts.indexer_pool msrc21_Lab.pred 6

# echo $CURRENT_DIR
cd $CURRENT_DIR

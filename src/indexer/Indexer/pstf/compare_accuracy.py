SCORE_THRESHOLD = 0.7
TAG_FILE_PATH = "tagfiles/"
CLASS_NUM = 22
CLASSES = [
        "total",
        "blding",
        "grass",
        "tree",
        "cow",
        "sheep",
        "sky",
        "plane",
        "water",
        "face",
        "car",
        "bicycle",
        "flower",
        "sign",
        "bird",
        "book",
        "chair",
        "road",
        "cat",
        "dog",
        "body",
        "boat"
    ]
order_mapping = [0, 7, 11, 14, 21, 20, 15, 1, 10, 18, 16, 4, 19, 9, 12, 2, 17, 5, 13, 6, 3, 8]
EPS = 1e-10

def get_tags(filename):
    tags_file = open(filename, 'r')
    tags = {}
    for line in tags_file:
        words = line.strip().split(',')
        key_frame_counter = int(words[0])
        scores = words[1:-1]
        for score_index in xrange(len(scores)):
            scores[score_index] = float(scores[score_index])
        tags[key_frame_counter] = scores
    tags_file.close()
    return tags

def detect_accuracy(tags_fact, tags_test, THRESHOLD):
    total_tags = []
    for i in range(CLASS_NUM):
        total_tags.append(0)
    TP = []
    for i in range(CLASS_NUM):
        TP.append(0)
    FP = []
    for i in range(CLASS_NUM):
        FP.append(0)

    for key, scores in tags_fact.items():
        test_scores = tags_test[key]
        for score_index, score in enumerate(scores):
            if score >= SCORE_THRESHOLD:
                total_tags[score_index] += 1
                if test_scores[score_index] >= THRESHOLD:
                    TP[score_index] += 1
            else:
                if test_scores[score_index] >= THRESHOLD:
                    FP[score_index] += 1
    return (total_tags, TP, FP)

def add_list(list_A, list_B):
    for i in range(len(list_A)):
        list_A[i] += list_B[i]
    return list_A

def main():
    TP_720 = []
    for i in range(CLASS_NUM):
        TP_720.append(0)
    FP_720 = []
    for i in range(CLASS_NUM):
        FP_720.append(0)
    TP_480 = []
    for i in range(CLASS_NUM):
        TP_480.append(0)
    FP_480 = []
    for i in range(CLASS_NUM):
        FP_480.append(0)
    TP_360 = []
    for i in range(CLASS_NUM):
        TP_360.append(0)
    FP_360 = []
    for i in range(CLASS_NUM):
        FP_360.append(0)
    total_tags = []
    for i in range(CLASS_NUM):
        total_tags.append(0)

    total_frames = 0

    for video_id in [1,2,3,6,7,9,10,11,12,13,15,16]:#range(1, 17):
        tags_fact = get_tags(TAG_FILE_PATH + str(video_id) + '_fact.txt')
        tags_720 = get_tags(TAG_FILE_PATH + str(video_id) + '_720.txt')
        tags_480 = get_tags(TAG_FILE_PATH + str(video_id) + '_480.txt')
        tags_360 = get_tags(TAG_FILE_PATH + str(video_id) + '_360.txt')

        result = detect_accuracy(tags_fact, tags_720, SCORE_THRESHOLD)
        TP_720 = add_list(TP_720, result[1])
        FP_720 = add_list(FP_720, result[2])
        result = detect_accuracy(tags_fact, tags_480, SCORE_THRESHOLD)
        TP_480 = add_list(TP_480, result[1])
        FP_480 = add_list(FP_480, result[2])
        result = detect_accuracy(tags_fact, tags_360, SCORE_THRESHOLD)
        TP_360 = add_list(TP_360, result[1])
        FP_360 = add_list(FP_360, result[2])

        total_tags = add_list(total_tags, result[0])
        total_frames += len(tags_fact)

    total_tags[0] = sum(total_tags)
    TP_720[0] = sum(TP_720)
    FP_720[0] = sum(FP_720)
    TP_480[0] = sum(TP_480)
    FP_480[0] = sum(FP_480)
    TP_360[0] = sum(TP_360)
    FP_360[0] = sum(FP_360)

    accuracy_file = open('indexingaccuracy.dat', 'w')
    #accuracy_file.write('CLASSES\t\tTOTAL_TAGS\tERROR_720\tERROR_480\tERROR_360\n')
    print 'CLASSES\t\tTOTAL_TAGS\tTP_720\t\tFP_720\tTP_480\t\tFP_480\tTP_360\t\tFP_360'
    for i in range(CLASS_NUM):
	ii = order_mapping[i]
        for_print = '%s\t\t%d\t\t%d(%.1f%%)\t%d\t%d(%.1f%%)\t%d\t%d(%.1f%%)\t%d' % (CLASSES[ii], total_tags[ii], 
                                                          TP_720[ii], TP_720[ii] / (total_tags[ii] + EPS) * 100, FP_720[ii], 
                                                          TP_480[ii], TP_480[ii] / (total_tags[ii] + EPS) * 100, FP_480[ii],
                                                          TP_360[ii], TP_360[ii] / (total_tags[ii] + EPS) * 100, FP_360[ii])
        for_write = '%s & %d & %d\\scriptsize{\\phantom{0}(%.1f\\%%)} & %d & %d\\scriptsize{\\phantom{0}(%.1f\\%%)} & %d & %d\\scriptsize{\\phantom{0}(%.1f\\%%)} & %d \\\\\n' % (CLASSES[ii], total_tags[ii],
                                                          TP_720[ii], TP_720[ii] / (total_tags[ii] + EPS) * 100, FP_720[ii],
                                                          TP_480[ii], TP_480[ii] / (total_tags[ii] + EPS) * 100, FP_480[ii],
                                                          TP_360[ii], TP_360[ii] / (total_tags[ii] + EPS) * 100, FP_360[ii])
        accuracy_file.write(for_write)
        print(for_print)
    accuracy_file.close()
    print "total frames: %d, total tags: %d" % (total_frames, total_tags[0])
    
if __name__ == '__main__':
    main()

import java.net.InetAddress;


public class ExperimentConfiguration {

	public final static String [] availableRes = {"360p","720p","1080p"};
	public final static String [] availableVideos = {"360p_ColIseranDescent","360p_DoncasterRailwayStation","360p_DriveLondonCity","360p_StreetParadeZurich","1080p_AalstCarnival","1080p_DrivingWestSide","1080p_NewYorkMarathon","1080p_SingaporeCityTour","480p_3_Pittsburgh","480p_5_Pittsburgh","480p_7_Walnut","1080p_2_Pittsburgh","1080p_4_Pittsburgh","1080p_6_Pittsburgh","1080p_2_Pittsburgh_1fps","1080p_4_Pittsburgh_1fps","1080p_6_Pittsburgh_1fps","480p_3_Pittsburgh_1fps","480p_5_Pittsburgh_1fps","480p_7_Walnut_1fps"};
	public final static String [] availableChunk = {"5 sec","30 sec", "5 min"};
	public final static int [] availableChunkSec = {5, 30, 5*60};
	public final static String baseDir = "/home/pieter/crawls/chopped";
	public static String resolution;
	public static String video;
	public static int chunk_sec;
	public static int no_iters;
	public static int no_nodes_max;
	public static int no_nodes;
	public static int no_nodes_min;
	public static int duration_sec = 10*60;
	public static boolean bDoUpload;
	public static boolean bLoopUpload;
	public static int DEFAULT_VIDEO_UPLOADPORT = 8080;
	public static int DEFAULT_CONTROL_UPLOADPORT = 8081;
	public static String DEFAULT_VIDEO_UPLOADIP = "192.168.2.12";
	public static String REFERENCE_NODE = "192.168.2.7";
	public static String ENERGYMON_IP = "128.237.236.164";
	public static String [] experimentDescription = {"All nodes upload same video", "All but one 480p","All but one 1080p","Random all but one 480p","Random all but one 1080p","Random all but one 480p_1fps","Random all but one 1080p_1fps","All but one 480p_1fps","All but one 1080p_1fps","Slotted all but one 480p","Slotted all but one 1080p","Slotted all but one 480p_1fps","Slotted all but one 1080p_1fps"};
	public static int ALL_IDENTICAL = 0;
	public static int ALLBUTONE_480p = 1;
	public static int ALLBUTONE_1080p = 2;
	public static int RANDOM_ALLBUTONE_480p = 3;
	public static int RANDOM_ALLBUTONE_1080p = 4;
	public static int RANDOM_ALLBUTONE_480p_1fps = 5;
	public static int RANDOM_ALLBUTONE_1080p_1fps = 6;
	public static int ALLBUTONE_480p_1fps = 7;
	public static int ALLBUTONE_1080p_1fps = 8;
	public static int SLOTTED_ALLBUTONE_480p = 9;
	public static int SLOTTED_ALLBUTONE_1080p = 10;
	public static int SLOTTED_ALLBUTONE_480p_1fps = 11;
	public static int SLOTTED_ALLBUTONE_1080p_1fps = 12;

	
	public static int experimentType;
	
	public static int defvideo = 8;
	public static int defExpType = 3;
}

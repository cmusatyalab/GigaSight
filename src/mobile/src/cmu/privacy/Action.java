package cmu.privacy;

/**
* GigaSight - CMU 2012
* @author Pieter Simoens
* 
*/ 

public enum Action {

	BLANK{
		public String toString(){
			return "blank";
		}
	},
	BLUR{
		public String toString(){
			return "blur";
		}		
	},
	PUBLISH{
		public String toString(){
			return "publish";
		}
	};

}

%THIS IS A MATLAB SCRIPT
basefilename='energymeasurements/energy_480p_3_Pittsburgh_30_';
results = 0;
disp('Calculation started');
for nodes = 1:8
	for iter = 0:4
 		filename = strcat(basefilename,num2str(nodes),'_',num2str(iter),'.csv');
 		if exist(filename) > 0
			disp(strcat('Parsing',filename))
  			M = csvread(filename,1,2);
			results(nodes,iter+1) = mean (M(:,1))
		else
			disp(strcat(filename,'not found'))
		end
	end
end
results

	

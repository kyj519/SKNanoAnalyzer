#SKFlat.py -a Vcb_FH -i 'QCD_HT70to100,QCD_HT100to200,QCD_HT200to400' -n 1 --userflags "Skim" -e 2022EE --skimming_mode
#SKFlat.py -a Vcb_FH -i 'TTJJ*,JetMET*,QCD_HT400to600,QCD_HT600to800,QCD_HT800to1000,QCD_HT1000to1200,QCD_HT1200to1500,QCD_HT1500to2000,QCD_HT2000toInf' -n -1 --userflags "Skim" -e 2022EE --skimming_mode
SKFlat.py -a Vcb_SL -i "Skim_Vcb_SL_*" -n -1 -e 2022EE --nmax 600 --userflags El
SKFlat.py -a Vcb_SL -i "Skim_Vcb_SL_*" -n -1 -e 2022EE --nmax 600 --userflags Mu



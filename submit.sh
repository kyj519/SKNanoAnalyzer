#!/bin/bash
#SKFlat.py -a MeasureJetTaggingR --userflags SL -i 'Skim_Vcb_SL*' -e 2022EE -n -1 --nmax 500
#SKFlat.py -a MeasureJetTaggingR --userflags DL -i 'Skim_Vcb_DL*' -e 2022EE -n -1 --nmax 500
#SKFlat.py -a MeasureJetTaggingR --userflags FH -i 'Skim_Vcb_FH*' -e 2022EE -n -1 --nmax 500
#SKFlat.py -a Vcb_DL -i 'TTLJ*,ST*,QCD_*,DYJets,WJets,ZZ*,WZ*,WW*,tt*' -n -10 -e 2022EE --userflags Skim --skimming_mode
#SKFlat.py -a Vcb_SL -i 'TTLL*,ST*,QCD_*,DYJets,WJets,ZZ*,WZ*,WW*,tt*' -n -10 -e 2022EE --userflags Skim --skimming_mode
#SKFlat.py -a Vcb_DL -i 'TTLL*,Muon,EGamma' -n -1 -e 2022EE --userflags Skim --skimming_modSKFlat.py -a Vcb_DL -i 'TTLL*,Muon,EGamma' -n -1 -e 2022EE --userflags Skim --skimming_modee
#SKFlat.py -a Vcb_SL -i 'TTLJ*,Muon,EGamma' -n -1 -e 2022EE --userflags Skim --skimming_mode
#SKFlat.py -a Vcb_FH -i 'JetMET*,QCD_*,TTJJ*' -n -1 -e 2022EE --userflags Skim --skimming_mode

#SKFlat.py -a Vcb_DL -i 'Skim_Vcb_DL*' -n -10 --userflags MM -e 2022EE --nmax 8000
#SKFlat.py -a Vcb_DL -i 'Skim_Vcb_DL*' -n -10 --userflags ME -e 2022EE --nmax 8000
#SKFlat.py -a Vcb_DL -i 'Skim_Vcb_DL*' -n -10 --userflags EE -e 2022EE --nmax 8000
SKFlat.py -a Vcb_SL -i 'Skim_Vcb_SL*' -n -1 --userflags El -e 2022EE --nmax 8000
SKFlat.py -a Vcb_SL -i 'Skim_Vcb_SL*' -n -1 --userflags Mu -e 2022EE --nmax 8000
#SKFlat.py -a Vcb_FH -i 'Skim_Vcb_FH*' -n -1 -e 2022EE
#SKFlat.py -a Vcb_FH -i 'Skim_Vcb_FH*' -n -1 --userflags Training -e 2022EE 
#SKFlat.py -a Vcb_FH -i 'QCD_HT70to100,QCD_HT100to200,QCD_HT200to400' -n -10 --userflags "Skim" -e 2022EE --skimming_modeSKFlat.py -a Vcb_FH -i 'QCD_HT70to100,QCD_HT100to200,QCD_HT200to400' -n -10 --userflags "Skim" -e 2022EE --skimming_mode
#SKFlat.py -a Vcb_FH -i 'TTJJ*,JetMET*,QCD_HT400to600,QCD_HT600to800,QCD_HT800to1000,QCD_HT1000to1200,QCD_HT1200to1500,QCD_HT1500to2000,QCD_HT2000toInf' -n -1 --userflags "Skim" -e 2022EE --skimming_mode
#SKFlat.py -a Vcb_FH -i 'QCD_HT400to600,QCD_HT600to800,QCD_HT800to1000,QCD_HT1000to1200,QCD_HT1200to1500,QCD_HT1500to2000,QCD_HT2000toInf' -n -1 --userflags "Skim" -e 2022EE --skimming_mode

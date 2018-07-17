tries=${21-3}
max=50
n="0"
base=`pwd`

sh script.conf "$@"
period=${5-60}
periodns3=$((1000*$period))
cd $1

#if [ -f ../results/$1.log ] && [ "$(head -n 1 ../results/$1.log)" != "Partial:" ]; then echo "Simulation done!"; exit; fi


echo "Partial:" > ../results/$1.log
echo "#n errorc waistdelayavg waistdelaystd waistdeliveryavg waistdeliverystd ns3jitteravg ns3jitterstd coojajitteravg coojajitterstd appdeliveryavg appdeliverystd appdelayavg appdelaystd" >> ns3final.log

while [ $n -le $max ]
do
  errorc=0
  increment=${13-123123}
  i="0"
  while [ $i -lt $tries ]
  do
    sed -e s/SCRIPT_SEED/"$increment"/g < ${1}-preseed.csc > ${1}.csc
    if [ -f log_error_$(($n))_$(($i)).log ]; then
    	echo skip $i seed $increment
        rm container.id
    else
      mkdir backup
      cp *.log backup
      cd ..
      make sim SIM=$1 NS3_SEED=$increment NS3_SIM=$3 NS3_ARGS="$4 --noCooja=$n --interPacketInterval=$periodns3"
      cd $1
      cat log_error.log
      if [ -f log_coojaerror.log ]; then
        echo "COOJA TIMEOUT!"
        cp log_coojaerror.log cooja_error_$n_$i
        rm *.log
        cp backup/* .
        rm -r backup
        continue	
      fi
      if [ -n "$(grep "SIG" log_error.log)" ]; then
        echo "ABORTED!"
        grep "SIG" log_error.log
        rm *.log
        cp backup/* .
        rm -r backup
        errorc=$((errorc+1))
        i=$((i+1))
        continue	
      fi
      rm -r backup
      if [ -f ../default.flux ]; then
        cnt=0
        echo "Parsing flux captures..."
        while read c; do
          if [ $cnt -lt $n ]; then
            ../parse_tap.py $c >> log_waistflux.log
            echo $c | awk '{print $1,$4}'
            ../parse_tap.py $(echo $c | awk '{print $1,$4}') >> log_capturedelay.log
            cnt=$((cnt+1))
          else
            break
          fi
        done < ../default.flux
      else
        ../parse_tap.py cot2 nst2 nst3 cot3 >> waist_$n.log
      fi
      awk -f ../final.awk log_waistflux.log >> waist_$n.log
      awk -f ../final.awk log_capturedelay.log >> capturedelay_$n.log
      cat log_jitter.log >> jitter_$n.log
      eval `echo rename "'s/log_([A-Za-z0-9]*).log/log_"'$1'"_${n}_$i.log/'" *.log`
      increment=$(($increment+10))
    fi
    i=$((i+1))
    echo "$n $i" > iteration
  done
  mv final.log final_$n.log
  waistdelayavg=$(awk -f ../final.awk waist_$n.log | grep Delay | awk '{print $2}')
  waistdelaystd=$(awk -f ../final.awk waist_$n.log | grep Delay | awk '{print $3}')
  waistdeliveryavg=$(awk -f ../final.awk waist_$n.log | grep Delivery | awk '{print $2}')
  waistdeliverystd=$(awk -f ../final.awk waist_$n.log | grep Delivery | awk '{print $3}')
  ns3jitteravg=$(awk -f ../final.awk jitter_$n.log | grep Ns3 | awk '{print $2}')
  ns3jitterstd=$(awk -f ../final.awk jitter_$n.log | grep Ns3 | awk '{print $3}')
  coojajitteravg=$(awk -f ../final.awk jitter_$n.log | grep Cooja | awk '{print $2}')
  coojajitterstd=$(awk -f ../final.awk jitter_$n.log | grep Cooja | awk '{print $3}')
  appdeliveryavg=$(awk -f ../final.awk final_$n.log | grep pdr | awk '{print $2}')
  appdeliverystd=$(awk -f ../final.awk final_$n.log | grep pdr | awk '{print $3}')
  appdelayavg=$(awk -f ../final.awk final_$n.log | grep delay | awk '{print $2}')
  appdelaystd=$(awk -f ../final.awk final_$n.log | grep delay | awk '{print $3}')
  
  if [ $errorc -lt $tries ]; then
    echo "$n $errorc $waistdelayavg $waistdelaystd $waistdeliveryavg $waistdeliverystd $ns3jitteravg $ns3jitterstd $coojajitteravg $coojajitterstd $appdeliveryavg $appdeliverystd $appdelayavg $appdelaystd" >> ns3final.log
  else
    echo "$n $errorc 0 0 0 0 0 0 0 0 0 0 0 0" >> ns3final.log
  fi
  echo "Partial:" > ../results/$1.log
  cat ns3final.log >> ../results/$1.log
  n=$((n+5))
  if [ $n -eq "51" ]; then
    n="50"
  fi
done
cat final.log > ../results/$1.log

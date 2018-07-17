{
  if($2 != ""){
    cnt[$1]+=1;
    sum[$1]+=$2;
    el[$1][cnt[$1]]=$2;
  }
}END{
  for(i in cnt){ 
    if(cnt[i]==0){
      print i,"0","0";
      continue;
    }
    avg[i]=sum[i]/cnt[i];
    for(j=1;j<=cnt[i];j++){
      sumvar[i] += (avg[i] - el[i][j])^2;
    }
    print i,avg[i],1.96*sqrt(sumvar[i]/(cnt[i]-1))/sqrt(cnt[i]);
  }
  if(avg["Control"] != "" && avg["Data"] != ""){
    totaltraf = avg["Control"]+avg["Data"];
    ctr_std = sqrt(sumvar["Control"]/cnt["Control"])
    print "Overhead",avg["Control"]/totaltraf,ctr_std/totaltraf
  }
}

curl -s "http://localhost:8080?[1-500]" &
pidlist="$pidlist $!" 
curl -s "http://localhost:8080/index.html?[1-500]" &
pidlist="$pidlist $!" 
curl -s "https://localhost:8081?[1-500]" &
pidlist="$pidlist $!" 
curl -s "https://localhost:8081/index.html?[1-500]" &
pidlist="$pidlist $!" 

for job in $pidlist; do
  wait $job || let "FAIL+=1"
done

if [ "$FAIL" == "" ]; then
  printf "\n\n\n                                --==OK==--\n\n\n"
  printf "2.000 requests, fetching on both SSL, and non-SSL connections, \n"
  printf "both documents and folders, in 4 consecutive threads,\n"
  printf "was successfully executed in;\n"
else
  printf "\n\nFAIL!\n"
  printf "Failure count; ($FAIL)"
fi
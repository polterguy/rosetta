curl -s "http://localhost:8080/?list=true[1-500]" &
pidlist="$pidlist $!" 
curl -s "http://localhost:8080/index.html?[1-500]" &
pidlist="$pidlist $!" 
curl -s "https://localhost:8081/?list=true[1-500]" &
pidlist="$pidlist $!" 
curl -s "https://localhost:8081/index.html?[1-250]" &
pidlist="$pidlist $!" 
curl -s "https://localhost:8081/not-existing?[1-250]" &
pidlist="$pidlist $!"

for job in $pidlist; do
  wait $job || let "FAIL+=1"
done

if [ "$FAIL" == "" ]; then
  printf "\n\n\n                                --==OK==--\n\n\n"
  printf "2.500 requests, fetching on both SSL, and non-SSL connections, \n"
  printf "both documents and folders, in 5 consecutive threads,\n"
  printf "was successfully executed in;\n"
else
  printf "\n\nFAIL!\n"
  printf "Failure count; ($FAIL)"
fi
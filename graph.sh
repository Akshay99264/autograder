#!/bin/bash
#author Akshay Patidar 23M0792
echo "Generating graph."

#clients='2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30'
clients='1 2 3 5 10 13 15 16 17 19 20 22 24 28 29 34 35 38 40 45 49 52 55 57 58 60 64 65 67 68 69 70 72 74 76 78 79 80 84 88 92 94 96 99 100'; #201 202 203 204 205 206 207 208 210 215 220 225 230 235 240 245 250 255 260 265 270 275 280 285 290 295 300 305 310 315 320 325 330 335 340 345 350 355 360 365 370 375 380 385 390 395 400' #'2 10 20 40 50 100 150 200 250 300 350 400 450 500 550 600 650 700 750'; # 800 850 900 950 1000 1050 1100 1150 1200 1250 1300 1350 1400 1450 1500'
touch response.txt
> response.txt
touch throughput.txt
> throughput.txt
touch goodput.txt
>goodput.txt
touch error_rate.txt
>error_rate.txt
touch timeout.txt
>timeout.txt

for i in ${clients};
do
  ./loadtest.sh $i 5 1 &
  wait
done

cat response.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Avg. Response time vs Clients" -X "Number of clients" -Y "Avg response time(in u_sec)"  -r 0.25> ./response.png
cat throughput.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Throughput vs Clients" -X "Number of clients" -Y "Throughput(Requests/u_sec)"  -r 0.25> ./throughput.png
cat goodput.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Goodput vs Clients" -X "Number of clients" -Y "Goodput"  -r 0.25> ./goodput.png
cat error_rate.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Error rate vs Clients" -X "Number of clients" -Y "Error rate"  -r 0.25> ./Error_rate.png
cat timeout.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Timeout rate vs Clients" -X "Number of clients" -Y "Timeout rate"  -r 0.25> ./Timeout_rate.png

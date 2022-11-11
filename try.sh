a=1;
while [ $a -ne $2 ] 
do
$a > tmp2.txt;
`./test < tmp2.txt` >> result.txt;
a=$[$a+1];
done

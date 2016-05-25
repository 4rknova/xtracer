number=`cat VERSION`

declare -a part=( ${number//\./ } )
declare    new
declare -i carry=1

for (( CNTR=${#part[@]}-1; CNTR>=0; CNTR-=1 )); do
  len=${#part[CNTR]}
  new=$((part[CNTR]+carry))
  carry=0
  part[CNTR]=${new}
done
new="${part[*]}"
echo -e "${new// /.}" | tee VERSION



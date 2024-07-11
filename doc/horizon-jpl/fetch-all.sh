mkdir -p temp
cd temp

sed 's/OBJECT_ID/199/g' ../horizons-nn-any.cmd > horizons-01-mercury.cmd
sed 's/OBJECT_ID/299/g' ../horizons-nn-any.cmd > horizons-02-venus.cmd
sed 's/OBJECT_ID/399/g' ../horizons-nn-any.cmd > horizons-03-earth.cmd
sed 's/OBJECT_ID/499/g' ../horizons-nn-any.cmd > horizons-04-mars.cmd
sed 's/OBJECT_ID/599/g' ../horizons-nn-any.cmd > horizons-05-jupiter.cmd
sed 's/OBJECT_ID/699/g' ../horizons-nn-any.cmd > horizons-06-saturn.cmd
sed 's/OBJECT_ID/799/g' ../horizons-nn-any.cmd > horizons-07-uranus.cmd
sed 's/OBJECT_ID/899/g' ../horizons-nn-any.cmd > horizons-08-neptun.cmd
sed 's/OBJECT_ID/999/g' ../horizons-nn-any.cmd > horizons-09-pluto.cmd

curl -s -F format=text -F input=@horizons-01-mercury.cmd https://ssd.jpl.nasa.gov/api/horizons_file.api > horizons-01-mercury.res
curl -s -F format=text -F input=@horizons-02-venus.cmd https://ssd.jpl.nasa.gov/api/horizons_file.api > horizons-02-venus.res
curl -s -F format=text -F input=@horizons-03-earth.cmd https://ssd.jpl.nasa.gov/api/horizons_file.api > horizons-03-earth.res
curl -s -F format=text -F input=@horizons-04-mars.cmd https://ssd.jpl.nasa.gov/api/horizons_file.api > horizons-04-mars.res
curl -s -F format=text -F input=@horizons-05-jupiter.cmd https://ssd.jpl.nasa.gov/api/horizons_file.api > horizons-05-jupiter.res
curl -s -F format=text -F input=@horizons-06-saturn.cmd https://ssd.jpl.nasa.gov/api/horizons_file.api > horizons-06-saturn.res
curl -s -F format=text -F input=@horizons-07-uranus.cmd https://ssd.jpl.nasa.gov/api/horizons_file.api > horizons-07-uranus.res
curl -s -F format=text -F input=@horizons-08-neptun.cmd https://ssd.jpl.nasa.gov/api/horizons_file.api > horizons-08-neptun.res
curl -s -F format=text -F input=@horizons-09-pluto.cmd https://ssd.jpl.nasa.gov/api/horizons_file.api > horizons-09-pluto.res

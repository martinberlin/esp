<?php
$filename1 = "data/3.jpg";
$filename2 = "data/1.jpg";
echo "file read: $filename1 \n";
echo "md5_file: ". md5_file($filename1);echo "\n\n";

echo "file read: $filename2 \n";
echo "md5_file: ". md5_file($filename2);echo "\n\n";

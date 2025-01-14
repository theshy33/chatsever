# Author: gss
# mail: 2649137052@qq.com
# Created Time: 2025.1.14
set -x

rm -rf "$(pwd)"/build/*
cd "$(pwd)"/build && 
	cmake .. && 
	make


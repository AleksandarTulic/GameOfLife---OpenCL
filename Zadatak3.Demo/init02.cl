__kernel void init02(__global int *mat1, const __global int *mat2, const int y11, const int x1, const int y1, const int x2, const int y2, const int y22){
	int r = get_global_id(0);
	int c = get_global_id(1);

	if ( r > x2 || r < x1 || c > y2 || c < y1 ) return;

	int oneRow11 = (y11 % 30 == 0 ? y11 / 30 : y11 / 30 + 1);
	int oneRow22 = (y22 % 30 == 0 ? y22 / 30 : y22 / 30 + 1);

	int pos111 = r * oneRow11 + c / 30;
	int pos112 = 1 << (c % 30);

	int pos221 = (r - x1) * oneRow22 + (c - y1) / 30;
	int pos222 = 1 << ((c - y1) % 30);

	int value2 = (mat2[pos221] & pos222) >= 1 ? 1 : 0;

	if ( (mat1[pos111] & pos112) >= 1 ){
		pos112*=(-1);
		atomic_add(&mat1[pos111], pos112);
	}

	pos112 = value2 << (c % 30);
	atomic_add(&mat1[pos111], pos112);
}
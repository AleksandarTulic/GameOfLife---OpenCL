__kernel void init01(__global int *matrix, __global int *pX, __global int *pY, const int y){
	int i = get_global_id(0);

	if ( pX[i] == -1 ) return;
	
	int oneRow = (y % 30 == 0 ? y / 30 : y / 30 + 1);
	int pos1 = pX[i] * oneRow + pY[i] / 30;
	int pos2 = 1 << (pY[i] % 30);

	int value = matrix[pos1] & pos2;

	if ( value != 0 ){
		pos2*=(-1);
		atomic_add(&matrix[pos1], pos2);
	}
}
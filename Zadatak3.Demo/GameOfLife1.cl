__kernel void GameOfLife(const __global int *oldMatrix, __global int *newMatrix, const int xStart, const int yStart, const int x, const int y){
	int r = get_global_id(0);
	int c = get_global_id(1);

	r+=xStart;
	c+=yStart;

	if ( r == 0 || r == x - 1 || c == 0 || c == y - 1 ) return;

	int pX[9];
	int pY[9];

	pX[0] = r; pY[0] = c;
	pX[1] = r - 1; pY[1] = c - 1;
	pX[2] = r + 1; pY[2] = c + 1;
	pX[3] = r - 1; pY[3] = c + 1;
	pX[4] = r + 1; pY[4] = c - 1;
	pX[5] = r - 1; pY[5] = c;
	pX[6] = r + 1; pY[6] = c;
	pX[7] = r; pY[7] = c - 1;
	pX[8] = r; pY[8] = c + 1;

	int howMuch = 30;
	int oneRow = (y % howMuch == 0 ? y / howMuch : y / howMuch + 1);	

	int live = 0;
	bool flag = false;

	for (int i=0;i<9;i++){
		int pos1 = pX[i] * oneRow + pY[i] / 30;
		int pos2 = 1 << (pY[i] % 30);

		int value = oldMatrix[pos1] & pos2;

		if ( value == 0 && i == 0 ) flag = true;
		if ( value == 0 && i != 0 ) live++;
	}

	int pos1 = r * oneRow + c / 30;
	int pos2 = 1 << (c % 30);

	if ( (flag && live >= 2 && live <= 3) || (!flag && live == 3) ) {
		if ( !flag ){
			pos2*=(-1);
			atomic_add(&newMatrix[pos1], pos2);
		}
	}
	else {
		if ( flag ){
			atomic_add(&newMatrix[pos1], pos2);
		}
	}
}
#include "ciph.h"
#include "value.h"
#include <math.h>
#include "vector.h"







/*
inline DblArr2 multMM(DblArr2 &m1, DblArr2 &m2){
	DblArr2 m;
	double r;
	int i,j,k;

	m.size1=m1.size1;
	m.size2=m2.size2;
	m.v = (double*)Malloc(sizeof(double)*m.size1*m.size2);
	if(m1.size2 != m2.size1) error("unequal number: matrix*matrix");		
	for(i=0; i<m1.size1; i++){
		for(j=0; j<m2.size2; j++){	
			r=0;
			for(k=0; k<m1.size2; k++){
				r += m1.v[(m1.size2)*i+k] * m2.v[(m2.size2)*k+j];
			}
			m.v[i*m2.size2+j]=r;
		}
	}
	return m;
}

DblArray multMV(DblArr2 m1, DblArray v2){
	DblArray rv;
	double r;
	int i,j;
	double *v;
	if(m1.size2 != v2.size)  error("mult: unequal number mat*vec");	
	rv.size=m1.size1;
	v = (double*)Malloc(sizeof(double)*rv.size);
	for(i=0; i<m1.size1; i++){
		r=0;
		for(j=0; j<m1.size2; j++){
			r += m1.v[i*m1.size2+j] * v2.v[j];
		}
		v[i]=r;
	}
	rv.v = v;
	return rv;
}
*/





//from numerical recipie
#define TINY 1.0e-20

template<class T> inline void swap(T& a, T& b){
	T t=a; a=b; b=t;
}

#define SWAP(a,b) {temp=(a);(a)=(b);(b)=temp;}
/*
inline double emm(obj ar, int i, int j){
	assert(ar->type==tArray);
	return (ar->u.array.v[i])->u.dbl_arr.v[j];
}
*/
double gaussj(obj v[], int n){	//assumes an array of dblarr
	// 完全ピボット交換	from numerical recipe
	int *indxc,*indxr,*ipiv;
	double dum,temp, det=0;

	indxc = new int[n];// pivot の列
	indxr = new int[n];// pivot の行
	ipiv = new int[n];	//処理済みの行をマーク
	for(int i=0; i<n;i++) ipiv[i]=0;
	for(int i=0; i<n;i++){  //処理すべき列についての主ループ
		int icol,irow;
		double big=0.0;
		for(int j=0;j<n;j++)
			if(ipiv[j] != 1){
				double* b = udar(v[j]).v;
				for(int k=0; k<n; k++){ //kは行
					if(ipiv[k] == 0){
						if(!(fabs(b[k]) < big)){
							big=fabs(b[k]);
							irow=j;
							icol=k;
						}
					} else if (ipiv[k] > 1) error("gaussj: Singular Matrix-1");
				}
			}
		(ipiv[icol])++;
		
		if(irow != icol){
			obj temp;
			SWAP(v[irow], v[icol]);	//この交換の後(icol, icol)がpivotに
		}
		indxr[i] = irow;
		indxc[i] = icol;
		double* c = udar(v[icol]).v;
		if(c[icol] == 0.0) error("gaussj: Singular Matrix-2");
		double pivinv = 1.0/c[icol];
		c[icol] = 1.0;
		for(int l=0; l<n; l++) c[l] *= pivinv;
		for(int ll=0; ll<n; ll++)
			if(ll != icol){
				double* b = udar(v[ll]).v;
				dum = b[icol];
				b[icol] = 0.0;
				for(int l=0;l<n;l++) b[l] -= c[l]*dum;
			}
	}	//各列を処理する主ループの終わり
	for(int l=n-1;l>=0;l--){
		if(indxr[l] != indxc[l]){
			for(int k=0; k<n; k++) {
				double* b = udar(v[k]).v;
				SWAP(b[indxr[l]], b[indxc[l]]);
			}
		}
	}
	delete [] ipiv;
	delete [] indxr;
	delete [] indxc;
	return det;
}

double max_abs(double v[], int n){
	assert(n>=1);
	double m = fabs(v[0]);
	for(int i=1; i<n; i++) if(m < fabs(v[i])) m = fabs(v[i]);
	return m;
}

void gaussj2(obj v[], int n){	//不完全ピボット交換
	double* ma = new double[n];
	for(int i=0; i<n; i++) ma[i] = 1.0 / max_abs(udar(v[i]).v, n);

	int *ix = new int[n];
	for(int i=0; i<n; i++) ix[i] = i;
	for(int i=0; i<n; i++){
		// find pivot
		int col = 0;
		double m = (udar(v[i]).v[i])*ma[i];
		for(int j=i+1; j<n; j++) 
			if(m < udar(v[j]).v[i]*ma[j]) {
				col = j;
				m = udar(v[j]).v[i]*ma[j];
			};

	
	}
	
	delete [] ix;
	delete [] ma;
}



inline obj em(obj ar, int i){
	assert(ar->type==tArray);
	return uar(ar).v[i];
}

obj inv(obj v){
	int spflag=0;	// =1, if the matrix contains sparse vector.
	obj m = map_arr(toDblArr, v);
	int n = size(v);
	for(int i=0; i<n; i++) {
		if(size(em(m, i)) != n) error("non-square matrix");
	}
	gaussj(uar(m).v, n);
	return m;
}

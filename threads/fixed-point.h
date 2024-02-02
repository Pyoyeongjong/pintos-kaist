#define F (1<<14)

int conv_i_to_f(int);
int conv_f_to_i(int);
int conv_f_to_ni(int);
int addff(int x, int y);
int subff(int x, int y);
int addfi(int x, int n);
int subfi(int x, int n);
int muxff(int x, int y);
int muxfi(int x, int n);
int divff(int x, int y);
int divfi(int x, int n);

int conv_i_to_f(int n){
    return n*F;
}

int conv_f_to_i(int x){
    return x/F;
}

int conv_f_to_ni(int x){
    if (x >=0)
        return (x+F/2)/F;
    else
        return (x-F/2)/F;
}

int addff(int x, int y){
    return x+y;
}

int subff(int x, int y){
    return x-y;
}

int addfi(int x, int n){
    return x+conv_i_to_f(n);
}

int subfi(int x, int n){
    return x-conv_i_to_f(n);
}

int muxff(int x, int y){
    return ((int64_t) x) * y/F;
}

int muxfi(int x, int n){
    return x*n;
}

int divff(int x, int y){
    return ((int64_t) x) * F / y;
}

int divfi(int x, int n){
    return x/n;
}






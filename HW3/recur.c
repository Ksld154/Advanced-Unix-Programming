#include <stdio.h>

long long int recur(int n) {
    if(n <= 0) 
        return 0;
    else if(n == 1) 
        return 1;
    else 
        return 2*recur(n-1) + 3*recur(n-2);
}


int main() {

    // int a, res;
    // scanf("%d", &a);
    
    for (int i = 0; i < 30; i++){
        long long int res = recur(i);
        printf("(i, Ans): %2d %lld\n", i, res);
    }
    return 0;
}
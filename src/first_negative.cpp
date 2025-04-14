#include <bits/stdc++.h>
using namespace std;
vector<int>negative(vector<int>&arr,int k){
    int n =arr.size();
    vector<int>res;
    for(int i =0;i<n-k+1;i++){
        bool found =false;
        for(int j =0;j<k;j++){
            if(arr[i+j]<0){
                res.push_back(arr[i+j]);
                found =true;
                break;

            }

        }
    }
    return res;
    
  
    





}
//optimized version of same code 


int main()
{
    vector<int>arr = { 10,-5,-22,7,1,-10};
    int k = 3;
    vector<int> res = negative(arr, k);

    for(int x : res) {
        cout << x << " ";
    }

    return 0;
}
    







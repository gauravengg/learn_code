#include <bits/stdc++.h>
using namespace std;
int second_largest(vector<int>& nums) {
    int n = nums.size();
    if (n < 2) {
        return -1; // Not enough elements
    }
    int first = INT_MIN, second = INT_MIN;
    for (int i = 0; i < n; i++) {
        if (nums[i] > first) {
            second = first;
            first = nums[i];
        } else if (nums[i] > second && nums[i] != first) {
            second = nums[i];
        }
    }
    return second;
    

    
    
}  
int main() {
    vector<int> nums = {2,4,14,18,78,73};
    int second = second_largest(nums);
    cout<<"Second largest element is: " << second << endl;
    
    
    return 0;
}


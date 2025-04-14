#include<iostream>
using namespace std;
class Node{
    public:
    int data;
    Node* next;
    Node(int new_data){
        this->data = new_data;
        this->next = nullptr;


    }
};
void traverse (Node *head){
    
    Node * temp=head;
    while(temp){
        cout<<temp->data<<" ";
        temp=temp->next;


    }
    cout<<endl;
}
int main() {
  
    // Create a hard-coded linked list:
    // 10 -> 20 -> 30 -> 40
    Node* head = new Node(10);
    head->next = new Node(20);
    head->next->next = new Node(30);
    head->next->next->next = new Node(40);

    // Example of traversing the node and printing
    //int length =get_length(head);
    //cout << "Length of linked list: " << length<< endl;
    traverse(head);

    return 0;
}
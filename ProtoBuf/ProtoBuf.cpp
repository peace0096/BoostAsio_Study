#include <iostream>
#include "Protocol.pb.h"

using namespace std;

//int main()
//{
//    Protocol::TEST test;
//    test.set_id(10);
//    test.set_hp(100);
//
//    string* name1 = test.add_name();
//    name1->assign("qwer!");
//    string* name2 = test.add_name();
//    name2->assign("asdf!");
//
//    auto vectest = test.add_vec();
//    vectest->set_data(10);
//
//    int size = test.ByteSizeLong();
//
//    // 직렬화
//    vector<char> vector(size);
//    test.SerializeToArray(vector.data(), size);
//
//    // 역직렬화
//    Protocol::TEST test2;
//    test2.ParseFromArray(vector.data(), size);
//
//    cout << test2.hp() << endl;
//    cout << test2.id() << endl;
//
//    // 첫번째 요소
//    // 인덱스 아웃오브 뜰 수도 있으니 유의
//    for (auto n : test.name())
//    {
//        cout << n << endl;
//    }
//
//    for (auto v : test.vec())
//    {
//        cout << v.data() << endl;
//    }
//
//    return 0;
//}
compileMex
%%
clear,clc

data.id=uint32(65);
data.name='Samsung Galaxy';
data.price=299.9;
data.rowUint=uint8([1 2 3 4 5 6 7 8 9]);
data.rowDouble=[23 346 456 56];
data.rowSingle=single([23 346 456 56]);
data.rowUint32=uint32([1 2 3 4 5 6 7 8 9 157645]);
data.rowInt32=int32([-12545 2 3 4 5 6 7 8 9 154543]);
data.rowInt64=int64([-1234323523545 2 3 4 5 6 7 8 9 15]);
data.another.id=int64(-2353252334);
data.another.text='Some text';
data.another.cost=54.3;
data.another.row=[234 23 65];

encoded=binnEncode(data);
decoded=binnDecode(encoded)
compileMex
%%
clear,clc

data.id=uint32(65);
data.name='Samsung Galaxy';
data.price=299.9;
data.rowUint=uint8([1 2 3 4 5 6 7 8 9]);
data.rowDouble=[23 346 456 56];
data.columnDouble=[23 346 456 56].';
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
%% test cell array

data={};
data{1}=12;
data{2}=[12 321 345 54];
data{3}.id=234;
data{3}.name='test';
data{4}={34, 'string', [23 78 23]};
encoded=binnEncode(data);
decoded=binnDecode(encoded)
%% test cell array with numeric data

data={};
data{1}=12;
data{2}=43;
data{3}=32;
encoded=binnEncode(data);
decoded=binnDecode(encoded)
%% test vector

data=[23 346 456 56];
encoded=binnEncode(data);
decoded=binnDecode(encoded)
%% test scalar

data=23;
encoded=binnEncode(data);
decoded=binnDecode(encoded)
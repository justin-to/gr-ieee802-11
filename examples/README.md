For running the loopback:
In the hierarchical block there are 2 variable blocks to change the subcarriers used when transmitting and receiving. 

The blocks to change are occupied_subcarriers and num_data_subs. The num_data_subs blocks passes into functions 
how many data carriers are being used, while the occupied_subcarriers block determine the index the data subcarriers will occupy.

It will also be necessary to change the encoding in utils.cc and frame_equalizer.cc to account for the number of subcarriers 
being used. Right now we only support multiples of 3. There are comments in those files for what to change the values to when 
working with 24 subcarriers. 

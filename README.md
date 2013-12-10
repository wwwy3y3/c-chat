# 講解
server處理很多client情況下, 有thread, AIO, select()等方法		
我使用了select		

# usage
`make server`
然後用telnet去連server

`show` -> 顯示線上人數跟id

`talk 7 haha` -> 傳"haha" 給 user7

talk 後面接著使用者的id, 在接著要傳得訊息就可以傳遞過去

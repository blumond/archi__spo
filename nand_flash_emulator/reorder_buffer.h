
//
//reponse 할 것들을 모음
//request가 들어오면 해당 queue 자리를 만들어 놓는다.
//만약 현재 들어온 response가 head이면 바로 전달
//아니면 기다렸다가 전달

extern struct queue_type *reorder_buffer;

void alloc_reorder_buffer(struct ftl_request p_ftl_req);
void put_reorder_buffer(struct ftl_request p_ftl_req);
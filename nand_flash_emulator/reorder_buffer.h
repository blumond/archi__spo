
//
//reponse �� �͵��� ����
//request�� ������ �ش� queue �ڸ��� ����� ���´�.
//���� ���� ���� response�� head�̸� �ٷ� ����
//�ƴϸ� ��ٷȴٰ� ����

extern struct queue_type *reorder_buffer;

void alloc_reorder_buffer(struct ftl_request p_ftl_req);
void put_reorder_buffer(struct ftl_request p_ftl_req);
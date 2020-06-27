#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#define MAX_PAYLOAD_LENGTH 64 // bit
#define MAX_PACKET_BURST 4 // maximum # packet at once per interface

typedef struct packet_t 
{
    unsigned int src;
    unsigned int dst;
    char payload[MAX_PAYLOAD_LENGTH];
} packet;

typedef struct packet_node_t 
{
    packet pkt;
    struct packet_node_t* next;
} packet_node;

typedef struct packet_queue_t
{
    packet_node* head;
    packet_node* tail;
} packet_queue;

typedef struct router_interface_t
{
    int id;
    packet_queue input_queue;
    packet_queue output_queue;
    unsigned int net;
    unsigned int width;
} router_interface;

typedef struct router_interface_node_t
{
    router_interface interface;
    struct router_interface_node_t* next;
} router_interface_node;

typedef struct routing_entry_t
{
    unsigned int index;
    unsigned int net;
    unsigned int width;
    int if_id;
}routing_entry;

typedef struct routing_node_t
{
    routing_entry r_entry;
    struct routing_node_t* next;
}routing_node;


router_interface_node* init(int);
router_interface_node* init_interface_node(int);
void explore(router_interface_node*);
void explore_packet_queue(packet_node*, char*);
int get_4cbyte(unsigned int, int);
void enqueue_pkt(packet_queue*, packet_node*);
packet_node* dequeue_pkt(packet_queue*);
packet_node* generate_pkt(unsigned int, unsigned int);
unsigned int match_ipnet(unsigned int, unsigned int, unsigned int, short);
void print_ip(unsigned int);
void generate_traffic(router_interface_node*);
unsigned int generate_ip(unsigned int, unsigned int);
unsigned int get_nif(router_interface_node*);
unsigned int get_nre(routing_node*);
router_interface* get_ri_from_id(router_interface_node*, int);
unsigned int random_ip();
unsigned int char2uint(char*);
routing_node* add_routing_node(routing_node*, routing_node*);
routing_node* remove_rt(routing_node*, unsigned int);
void index_routing_table(routing_node*);
void print_rt(routing_node*);
unsigned int route(routing_node*, unsigned int);
routing_node* allocate_rtgn(unsigned int, unsigned int, unsigned int);
void generate_subnets(router_interface_node*);
routing_node* generate_rt(router_interface_node*);
void explore_subnets(router_interface_node*);
unsigned int get_netbase(unsigned int, unsigned int);
void routing_operation(router_interface_node*, routing_node*);
void service_operation(router_interface_node*);
void full_auto(unsigned int, router_interface_node*, routing_node*);
router_interface_node* add_if(router_interface_node*, int, unsigned int, unsigned int);
int menu();
int sub_manual_menu();
int insert_upkt(router_interface_node*);
void save_router_queue_state(router_interface_node*);
void save_routing_table(routing_node*);
void save_interfaces(router_interface_node*);
void load_router_queue_state(router_interface_node*);
routing_node* load_routing_table();
router_interface_node* load_interfaces();
void discard_conf();

int main(){
    int res;
    do
    {
        res = menu();
    }while(res >= 0);
    return 0;
}

int menu()
{
    int choice, nif, i, mask, rt_entry, if_t, loaded = 0, rt_loaded = 0;
    router_interface_node* if_lst = NULL;
    routing_node* routing_table = NULL;
    char net[15];
    if_lst = load_interfaces();
    printf("\n>Benvenuto nel simulatore di router!\n\n");
    if(if_lst != NULL)
    {
        load_router_queue_state(if_lst);
        if((routing_table = load_routing_table()) != NULL)
            rt_loaded = 1;
        loaded = 1;
        printf("\n>Configurazione Esistente caricata con successo!\n");
    } else
    {
        printf("\n>Nessuna configurazione trovata!\n\n");
        loaded = 0;
        rt_loaded = 0;
    } 
    printf("1 - Modalità automatica\n");
    printf("2 - Modalità manuale\n");
    printf("3 - Cancella Configurazione\n");
    printf("0 - Esci\nScegli: ");
    scanf("%d", &choice);
    switch(choice)
    {
        case 1:
            printf("-----> Modalità AUTO <-----\n\n");
            if(!loaded)
            {
                printf("Inserisci il numero di interfacce del router: ");
                scanf("%d", &nif); getchar();
                if(nif<= 0)
                {
                    printf("Il numero di interfacce deve essere > 0!\n");
                    return 0;
                }
            }
            full_auto(nif, if_lst, routing_table);
            return 0;
        case 2:
            printf("-----> Modalità MANUALE <-----\n\n");
            if(!loaded)
            {
                printf("Inserisci il numero di interfacce del router: ");
                scanf("%d", &nif); getchar();
                if(nif<= 0)
                {
                    printf("Il numero di interfacce deve essere >= 0!\n");
                    return 0;
                }
                for(i = 0; i< nif; i++)
                {
                    printf("Inserisci la sottorete alla quale e' collegata l'interfaccia [%d]\n>IP subnet: ", i);
                    scanf("%s", net);
                    do
                    {
                        printf("Inserisci ampiezza (intesa come log2 del numero di indirizzi IP contenuti nella subnet, 1 < ampiezza < 25): ");
                        scanf("%d", &mask);
                    } while (mask < 2 || mask > 24);
                    if_lst = add_if(if_lst, i, char2uint(net), mask);
                    memset(net, '\0', 15*sizeof(char));
                }
                save_interfaces(if_lst);
            }
            printf("Riepilogo:\n");
            explore_subnets(if_lst);
            printf("\n");
            if(!rt_loaded)
            {
                printf("Genero Tabella di routing..\n");
                routing_table = generate_rt(if_lst);
            }
            print_rt(routing_table);
            save_routing_table(routing_table);
            while((choice = sub_manual_menu()) != 0)
            {
                switch(choice)
                {
                    case 1:
                        routing_operation(if_lst, routing_table);
                        printf("\n");
                        explore(if_lst);
                        printf("\n");
                        service_operation(if_lst);
                        printf("\n");
                        explore(if_lst);
                        break;
                    case 2:
                        if(insert_upkt(if_lst))
                        {
                            printf("\nPacchetto incodato.. Riepilogo code:\n");
                            explore(if_lst);
                            printf("\n");
                        } 
                        save_router_queue_state(if_lst);   
                        break;
                    case 3:
                        printf("Inserisci la sottorete alla quale e' collegata l'interfaccia [%d]\n>IP subnet: ", get_nif(if_lst));
                        scanf("%s", net);
                        printf("Inserisci ampiezza (intesa come log2 del numero di indirizzi IP contenuti nella subnet, ampiezza massima = 24): ");
                        scanf("%d", &mask);
                        if_lst = add_if(if_lst, get_nif(if_lst), char2uint(net), mask);
                        memset(net, '\0', 15*sizeof(char));
                        printf("Riepilogo:\n");
                        explore_subnets(if_lst);
                        printf("\n");
                        printf("Aggiorno Tabella di routing..\n");
                        routing_table = generate_rt(if_lst);
                        print_rt(routing_table);
                        save_routing_table(routing_table);
                        break;
                    case 4:
                        printf("Inserisci indirizzo IP della sottorete: ");
                        scanf("%s", net);
                        printf("Inserisci ampiezza (intesa come log2 del numero di indirizzi IP contenuti nella subnet, ampiezza massima = 24): ");
                        scanf("%d", &mask);
                        printf("Inserisci Interfaccia destinazione: ");
                        scanf("%d", &if_t);
                        if(if_t < 0 || if_t >= get_nif(if_lst))
                        {
                            printf("Interfaccia di destinazione non valida..\n");
                            memset(net, '\0', 15*sizeof(char));
                            break;
                        }
                        routing_table = add_routing_node(routing_table, allocate_rtgn(if_t, char2uint(net), mask));
                        print_rt(routing_table);
                        save_routing_table(routing_table);
                        memset(net, '\0', 15*sizeof(char));
                        break;
                    case 5:
                        print_rt(routing_table);
                        printf("Seleziona l'indice della riga da eliminare (campo estrema sinistra): ");
                        scanf("%d", &rt_entry);
                        if(rt_entry < 0 || rt_entry >= get_nre(routing_table))
                        {
                            printf("Indice di riga non valido..\n");
                            break;
                        }
                        routing_table = remove_rt(routing_table, rt_entry);
                        printf("Riepilogo Routing Table..\n");
                        print_rt(routing_table);
                        save_routing_table(routing_table);
                        break;
                    case 6:
                        routing_operation(if_lst, routing_table);
                        save_router_queue_state(if_lst);
                        printf("\n");
                        explore(if_lst);
                        printf("\n");
                        break;
                    case 7:
                        service_operation(if_lst);
                        save_router_queue_state(if_lst);
                        printf("\n");
                        explore(if_lst);
                        break;
                    case 8:
                        printf("\n");
                        explore(if_lst);
                        printf("\n");
                        break;
                    case 9:
                        print_rt(routing_table);
                        break;
                }
            }
            return 0;
        case 3:
            discard_conf();
            rt_loaded = 0;
            loaded = 0;
            return 0;
        case 0:
            return -1;
        default:
            printf("La scelta non e' valida..\n");
            return 0;
    }
    return -1;
}

void discard_conf()
{
    /*
    FILE* fp;
    fp = fopen("interfaces_data.txt", "w");
    fclose(fp);
    fp = fopen("routing_table.txt", "w");
    fclose(fp);
    fp = fopen("queue_state.txt", "w");
    fclose(fp);
    */
    remove("interfaces_data.txt");
    remove("routing_table.txt");
    remove("queue_state.txt");
    printf("\n>Configurazione Cancellata..\n\n");
}

int insert_upkt(router_interface_node * head)
{
    char src[15];
    char dst[15];
    int if_id;
    packet_node* pkt_n;
    printf("Inserisci sorgente pacchetto: ");
    scanf("%s", src); getchar();
    printf("Inserisci destinazione pacchetto: ");
    scanf("%s", dst); getchar();
    printf("Inserisci Interfaccia di provenienza: ");
    scanf("%d", &if_id);
    if(if_id <0 || if_id > (get_nif(head)-1))
    {
        printf("ERR: Identificatio Interfaccia non valida..\n");
        return 0;
    }
    while(head != NULL)
    {
        if(head->interface.id == if_id)
        {
            enqueue_pkt(&head->interface.input_queue, generate_pkt(char2uint(src), char2uint(dst)));
            return 1;
        }
        head = head->next;
    }
}

int sub_manual_menu()
{
    int choice;
    printf("\n");
    printf("1 - Start Complete Simulation (Routing + Service)\n");
    printf("2 - Inserisci pacchetto in arrivo\n");
    printf("3 - Aggiungi interfaccia router\n");
    printf("4 - Aggiungi regola nella Tabella di routing\n");
    printf("5 - Elimina regola dalla Tabella di routing\n");
    printf("6 - Instrada pacchetti nelle code di Arrivo\n");
    printf("7 - Spedisci i pacchetti instradati\n");
    printf("8 - Stampa Code I/O\n");
    printf("9 - Stampa Tabella di Routing\n");
    printf("0 - Esci\n");
    printf("Scegli: ");
    scanf("%d", &choice); getchar();
    if(choice < 0 || choice > 9) printf("Scelta non valida..\n");
    printf("\n");
    return choice;
}

void full_auto(unsigned int nif, router_interface_node* iflst_head, routing_node* routing_table)
{
    if(iflst_head == NULL)
    {
        iflst_head = init(nif);
        generate_subnets(iflst_head);
        routing_table = generate_rt(iflst_head);
    } else if(routing_table == NULL) routing_table = generate_rt(iflst_head);
	explore_subnets(iflst_head);
	print_rt(routing_table);
    save_interfaces(iflst_head);
    save_routing_table(routing_table);
    printf(">Send any char to continue, 'x' to exit..\n\n");
    while(getchar() != 'x')
    {
        generate_traffic(iflst_head);
        explore(iflst_head);
        routing_operation(iflst_head, routing_table);
        printf("\n");
        explore(iflst_head);
        printf("\n");
        service_operation(iflst_head);
        printf("\n");
        explore(iflst_head);
        printf(">Send any char to continue, 'x' to exit..\n\n");
    }
}

void service_operation(router_interface_node* iflst_head)
{
    packet_node* pkt_n = NULL;
    printf("---->Send Packets\n");
    while(iflst_head != NULL)
    {
        while((pkt_n = dequeue_pkt(&iflst_head->interface.output_queue)) != NULL)
        {
            printf(">Send Packet with dst {"); print_ip(pkt_n->pkt.dst); printf("} from Interface [%d]\n", iflst_head->interface.id);
            free(pkt_n);
        }
        iflst_head = iflst_head->next;
    }
    printf("---->End\n");
}

void routing_operation(router_interface_node* iflst_head, routing_node* rt_head)
{
    packet_node* pkt_n = NULL;
    router_interface_node* temp_head;
    router_interface_node* copy_head = iflst_head;
    unsigned int if_target;
    printf("---->Routing\n");
    while(iflst_head != NULL)
    {
        while((pkt_n = dequeue_pkt(&iflst_head->interface.input_queue)) != NULL)
        {
            temp_head = copy_head;
            if_target = route(rt_head, pkt_n->pkt.dst);
            //printf("If_Target: %d\n", if_target);
            while(temp_head != NULL)
            {
                if(temp_head->interface.id == if_target)
                {
                    enqueue_pkt(&temp_head->interface.output_queue, pkt_n);
                    break;
                }
                temp_head = temp_head->next;
            }
        }
        iflst_head = iflst_head->next; 
    }
    printf("---->End\n");
}

packet_node* generate_pkt(unsigned int src, unsigned int dst)
{
  packet_node* pkt_node = (packet_node*)malloc(sizeof(packet_node));
  pkt_node->next = NULL;
  pkt_node->pkt.src = src;
  pkt_node->pkt.dst = dst;
  return pkt_node;
}

router_interface_node* init_interface_node(int id)
{
    router_interface_node* node = (router_interface_node*)malloc(sizeof(router_interface_node));
    node->interface.id = id;
    node->interface.input_queue.head = NULL;
    node->interface.input_queue.tail = NULL;
    node->interface.output_queue.head = NULL;
    node->interface.output_queue.tail = NULL;
    node->next = NULL;
    return node;
}

router_interface_node* init(int i)
{
    printf("Router simulator with: [%d] interfaces.\n", i);
    srand(time(NULL));
    router_interface_node* node;
    router_interface_node* temp = NULL;
    router_interface_node* head = NULL;
    for(int j = 0; j < i; j++)
    {
        node = init_interface_node(j);
        if(head != NULL)
        {
            temp->next = node;
            temp = node;
        } else 
        {
            head = node;
            temp = node;
        }
    }
    return head;
}

router_interface_node* add_if(router_interface_node* if_lst, int id, unsigned int net, unsigned int width)
{
    router_interface_node* node = init_interface_node(id);
    router_interface_node* temp = NULL;
    router_interface_node* head = if_lst;
    node->interface.net = net;
    node->interface.width = width;
    if(if_lst == NULL)
    {   
        if_lst = node;
        return if_lst;
    }
    while(if_lst != NULL)
    {
        temp = if_lst;
        if_lst = if_lst->next;
    }
    temp->next = node;
    return head;
}

void enqueue_pkt(packet_queue* pq, packet_node* pkt)
{
  pkt->next = NULL;
  if(pq->head == NULL)
  {
    pq->head = pkt;
    pq->tail = pkt;
  } else
  {
    pq->tail->next = pkt;
    pq->tail = pkt;
  }
}

packet_node* dequeue_pkt(packet_queue* pq)
{
  packet_node* pkt = pq->head;
  if(pq->head == NULL) return pkt;
  if(pq->head->next == NULL)
  {
    pq->head = NULL;
    pq->tail = NULL;
  } else
  {
    pq->head = pq->head->next;
  }
  pkt->next = NULL;
  return pkt;
}

int get_4cbyte(unsigned int ip, int index)
{
    int mask[] = {0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF};
    return (ip & mask[index]) >> 24 - (index*8);
}

void explore_packet_queue(packet_node* head, char* name)
{
    printf("Exploring [%s] queue\n", name);
    while(head != NULL)
    {
        printf(">Packet from: %d.%d.%d.%d -> ", get_4cbyte(head->pkt.src, 0), get_4cbyte(head->pkt.src, 1), get_4cbyte(head->pkt.src, 2) ,get_4cbyte(head->pkt.src, 3));
        printf("to: %d.%d.%d.%d\n", get_4cbyte(head->pkt.dst, 0), get_4cbyte(head->pkt.dst, 1), get_4cbyte(head->pkt.dst, 2) ,get_4cbyte(head->pkt.dst, 3));
        head = head->next;
    }
}

void explore(router_interface_node* head)
{
    while(head != NULL)
    {
        printf("Interface n: %d\n", head->interface.id);
        explore_packet_queue(head->interface.input_queue.head, "Input queue");
        explore_packet_queue(head->interface.output_queue.head, "Output queue");
        head = head->next; 
        printf("\n");
    }
}

void explore_subnets(router_interface_node* head)
{
	while(head != NULL)
    {
		printf("Interface [%d] belongs to:", head->interface.id); print_ip(head->interface.net); printf("/%d", (32 - head->interface.width));
		printf("\t"); print_ip(head->interface.net); printf("->"); print_ip(head->interface.net + (0x00000001 << (head->interface.width-1)));
		printf("\n");
		head = head->next;
	}
}

void print_ip(unsigned int ip)
{
  printf(" %d.%d.%d.%d ", get_4cbyte(ip, 0), get_4cbyte(ip, 1), get_4cbyte(ip, 2), get_4cbyte(ip, 3));
}

unsigned int match_ipnet(unsigned int ip, unsigned int network, unsigned int width, short print)
{	
    if(print){
        printf("Match:"); print_ip(ip); printf("to"); print_ip(network); printf("with mask:"); print_ip((0xFFFFFFFF << (width))); printf("\n");
    }
    return ((ip & (0xFFFFFFFF << (width))) == (network & (0xFFFFFFFF << (width))));
}

unsigned int get_netbase(unsigned int ip, unsigned int width)
{
  return (ip & (0xFFFFFFFF << width));
}

unsigned int get_nif(router_interface_node* head)
{
    int nif = 0;
    while(head != NULL)
    {
        nif++;
        head = head->next; 
    }
    return nif;
}

unsigned int get_nre(routing_node* head)
{
    int nre = 0;
    while (head != NULL)
    {
        nre++;
        head = head->next;
    }
    return nre;
}

unsigned int char2uint(char* ip)
{
    unsigned int temp = 0, index = 0, uint_ip = 0;
    int mul[] = {1, 10, 100};
    int digit[] = {-1, -1, -1};
    int k = 0, j = 0;
    while(*ip != '\0')
    {
        while(((*ip) != '.') && ((*ip) != '\0'))
        {
            digit[2-k] = *ip - '0';
            k++;
            ip++;
        }
        k = 0;
        for(j = 0; j< 3; j++)
        {
            if(digit[j] != -1)
            {
                temp += (digit[j]*mul[k]);
                k++;
            }
        }
        uint_ip += temp << 24 - (8*index);
        if((*ip) == '\0') break;
        ip++; k = 0; index++; temp = 0;
        memset(digit, -1, 3*sizeof(int));
    }
    return uint_ip;
}

unsigned int generate_ip(unsigned int net, unsigned int width)
{
    return (++net) + (rand()%(width-1));
}

unsigned int random_ip()
{
    return generate_ip(0x00000000, 0xFFFFFFFF);
}

router_interface* get_ri_from_id(router_interface_node* head, int id)
{
    while(head != NULL)
    {
        if(head->interface.id == id) return &head->interface;
        head = head->next;
    }
    return NULL;
}

void generate_traffic(router_interface_node* head)
{
    unsigned int nif, target_if, burst, mask, src, dst;
    router_interface* target_ri = NULL;
    router_interface_node* current_if = head;
    nif = get_nif(head);
    while(current_if != NULL)
    {
        while((target_if = rand()%nif) == current_if->interface.id);
        //printf("Source interface: [%d]\n", current_if->interface.id);
        //printf("Target interface: [%d]\n", target_if);
        burst = rand()% (MAX_PACKET_BURST + 1);
        //printf("Packet burst: %d - interface [%d] and width [%d]\n", burst, current_if->interface.id, current_if->interface.width);
        while(burst > 0)
        {
            burst--;
            if (rand()%3 != 0)
                src = generate_ip(current_if->interface.net, pow(2, current_if->interface.width));
            else 
                src = random_ip();
            if((target_ri = get_ri_from_id(head, (target_if))) != NULL)
            {
                if (rand()%3 != 0) dst = generate_ip(target_ri->net, pow(2, target_ri->width));
                else dst = random_ip();
                enqueue_pkt(&current_if->interface.input_queue, generate_pkt(src, dst));
            } else { printf("ERR: Target interface [%d] does not exist.\n", target_if); break; }
        }
        //printf("Src generated: "); print_ip(src); printf("\n");
        //printf("Dst generated: "); print_ip(dst); printf("\n");
        current_if = current_if->next; 
    }
}

routing_node* add_routing_node(routing_node* head, routing_node* node)
{
    routing_node* ptr = head;
    node->next = NULL;
    if(head == NULL)
    {
        head = node;
        node->next = NULL;
    } else
    {
        while(ptr->next != NULL && node->r_entry.width >= ptr->next->r_entry.width) 
        {
            ptr = ptr->next;
        }
        if(ptr == head)
        {
            if(ptr->r_entry.width >= node->r_entry.width) 
            {
                node->next = ptr;
                head = node;
            } else
            {
                node->next = ptr->next;
                ptr->next = node;
            }
        } else
        {
            node->next = ptr->next;
            ptr->next = node;
        }
    }
    index_routing_table(head);
    return head;
}

routing_node* remove_rt(routing_node* head, unsigned int index)
{
  routing_node* ptr_1 = head;
  routing_node* ptr_2 = head;
  while(ptr_1 != NULL)
  {
    if(ptr_1->r_entry.index == index)
    {
      if(ptr_1 == ptr_2)
      {
	head = ptr_1->next;
      } else
      {
	ptr_2->next = ptr_1->next;
      }
      break;
    }
    ptr_2 = ptr_1;
    ptr_1 = ptr_1->next;
  }
  index_routing_table(head);
  return head;
}

void index_routing_table(routing_node* head)
{
    
    if(head == NULL) return;
    int index = 0;
    while(head != NULL)
    {
        head->r_entry.index = index;
        index++;
        head = head->next;
    }
}

void print_rt(routing_node* head)
{
    printf("\nRouting Table<Index, Network, Mask, Interface target>:\n");
    while(head != NULL)
    {
        printf("|\t%d\t|\t\t", head->r_entry.index); 
        print_ip(head->r_entry.net); 
        printf("/%d\t\t\t", (32-head->r_entry.width)); 
        print_ip(0xFFFFFFFF << (head->r_entry.width)); printf("\t\t%d\t", head->r_entry.if_id);
        printf("\n");
        head = head->next;
    }
    printf("\n");
}

unsigned int route(routing_node* rt_ptr, unsigned int dst)
{
  while(rt_ptr != NULL)
  {
    if(match_ipnet(dst, rt_ptr->r_entry.net, (rt_ptr->r_entry.width), 0)) 
    {
        printf("Packet with dst {"); print_ip(dst); printf("} is routed to interface [%d]\n", rt_ptr->r_entry.if_id);
        return rt_ptr->r_entry.if_id;
    }
    rt_ptr = rt_ptr->next;
  }
  printf("Packet with dst {"); print_ip(dst); printf("} is routed to interface [%d] - Default Gateway\n", 0);
  return 0;
}

routing_node* allocate_rtgn(unsigned int if_id, unsigned int net, unsigned int width)
{
  routing_node* re = (routing_node*)malloc(sizeof(routing_node));
  re->r_entry.if_id = if_id;
  re->r_entry.net = net;
  re->r_entry.width = width;
  return re;
}

void generate_subnets(router_interface_node* head)
{
  unsigned int width;
  while(head != NULL)
  {
    width = rand()%3 + 2;
    head->interface.width = width;
    head->interface.net = get_netbase(random_ip(), width);
    head = head->next;
  }
}

routing_node* generate_rt(router_interface_node* head_ri)
{
	routing_node* routing_table = NULL;
    
	while(head_ri != NULL)
	{
		routing_table = add_routing_node(routing_table, allocate_rtgn(head_ri->interface.id, head_ri->interface.net, head_ri->interface.width));
		head_ri = head_ri->next;
	}
    
	return routing_table;
}

void save_router_queue_state(router_interface_node* head_ri)
{
    FILE* fp = fopen("queue_state.txt", "w");
    packet_node* temp = NULL;
    fprintf(fp, "%d\n", get_nif(head_ri));
    while(head_ri != NULL)
    {
        temp = head_ri->interface.input_queue.head;
        while(temp != NULL)
        {
            fprintf(fp, "%c %d %d.%d.%d.%d %d.%d.%d.%d\n", 'I', head_ri->interface.id, 
                                            get_4cbyte(temp->pkt.src, 0), get_4cbyte(temp->pkt.src, 1), get_4cbyte(temp->pkt.src, 2), get_4cbyte(temp->pkt.src, 3),
                                            get_4cbyte(temp->pkt.dst, 0), get_4cbyte(temp->pkt.dst, 1), get_4cbyte(temp->pkt.dst, 2), get_4cbyte(temp->pkt.dst, 3));
            temp = temp->next;
        }
        temp = head_ri->interface.output_queue.head;
        while(temp != NULL)
        {
            fprintf(fp, "%c %d %d.%d.%d.%d %d.%d.%d.%d\n", 'O', head_ri->interface.id, 
                                            get_4cbyte(temp->pkt.src, 0), get_4cbyte(temp->pkt.src, 1), get_4cbyte(temp->pkt.src, 2), get_4cbyte(temp->pkt.src, 3),
                                            get_4cbyte(temp->pkt.dst, 0), get_4cbyte(temp->pkt.dst, 1), get_4cbyte(temp->pkt.dst, 2), get_4cbyte(temp->pkt.dst, 3));
            temp = temp->next;
        }
        head_ri = head_ri->next;
    }
    fclose(fp);
}

void load_router_queue_state(router_interface_node* head_ri)
{
    int if_id, nif;
    char c, src[15], dst[15];
    packet_node* pkt_node;
    router_interface* temp = NULL;
    
    FILE* fp = fopen("queue_state.txt", "r");
    if(fp == NULL) return;
    while((fscanf(fp, "%c %d %s %s\n", &c, &if_id, src, dst)) != EOF)
    {
        temp = get_ri_from_id(head_ri, if_id);
        if(temp != NULL)
        {
            pkt_node = generate_pkt(char2uint(src), char2uint(dst));
            if(c == 'I')
            {
                enqueue_pkt(&temp->input_queue, pkt_node);
            } else if( c == 'O')
            {
                enqueue_pkt(&temp->output_queue, pkt_node);
            }
        }
    }
    fclose(fp);
}

void save_routing_table(routing_node* head_rt)
{
    FILE* fp = fopen("routing_table.txt", "w");
    while(head_rt != NULL)
    {
        fprintf(fp, "%d %d.%d.%d.%d %d %d\n", head_rt->r_entry.index,
                                              get_4cbyte(head_rt->r_entry.net, 0), get_4cbyte(head_rt->r_entry.net, 1), get_4cbyte(head_rt->r_entry.net, 2), get_4cbyte(head_rt->r_entry.net, 3),
                                              head_rt->r_entry.width, 
                                              head_rt->r_entry.if_id);
        head_rt = head_rt->next;
    }
    fclose(fp);
}

routing_node* load_routing_table()
{
    FILE* fp = fopen("routing_table.txt", "r");
    routing_entry r_entry;
    routing_node* routing_table = NULL;
    char net[15];
    if(fp == NULL) return NULL;
    while((fscanf(fp, "%d %s %d %d\n", &r_entry.index, net, &r_entry.width, &r_entry.if_id)) != EOF)
    {
        r_entry.net = char2uint(net);
        routing_table = add_routing_node(routing_table, allocate_rtgn(r_entry.if_id, r_entry.net, r_entry.width));
    }
    fclose(fp);
    return routing_table;
}

void save_interfaces(router_interface_node* head_if)
{
    FILE* fp = fopen("interfaces_data.txt", "w");
    fprintf(fp, "%d\n", get_nif(head_if));
    while(head_if != NULL)
    {
        fprintf(fp, "%d %d.%d.%d.%d %d\n", head_if->interface.id,
                                                  get_4cbyte(head_if->interface.net, 0), get_4cbyte(head_if->interface.net, 1), get_4cbyte(head_if->interface.net, 2), get_4cbyte(head_if->interface.net, 3),
                                                  head_if->interface.width);
        head_if = head_if->next;
    }
    fclose(fp);
}

router_interface_node* load_interfaces()
{
    int if_id, width, nif;
    char net[15];
    router_interface_node* head_ri = NULL;
    router_interface* temp = NULL;
    FILE* fp = fopen("interfaces_data.txt", "r");
    if(fp == NULL) return NULL;
    fscanf(fp, "%d\n", &nif);
    head_ri = init(nif);
    while((fscanf(fp, "%d %s %d\n", &if_id, net, &width)) != EOF)
    {
        temp = get_ri_from_id(head_ri, if_id);
        if(temp != NULL)
        {
            temp->net = char2uint(net);
            temp->width = width;
        }
    }
    fclose(fp);
    return head_ri;
}


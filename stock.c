#include "stock.h"

int main(int argc, char **argv) {
    sell_q = queue_create();
    buy_q = queue_create();
    trade_q = queue_create();
    cancel_q = queue_create();

    char signal[SIGNAL_BUF_SIZE];
    while (fgets(signal, SIGNAL_BUF_SIZE, stdin)) {
        parse_signal(signal);
    }

    fprintf(stderr, "Done\n");

    queue_destroy_complete(sell_q, NULL);
    queue_destroy_complete(buy_q, NULL);
    queue_destroy_complete(trade_q, NULL);
    queue_destroy_complete(cancel_q, NULL);

    exit(EXIT_SUCCESS);
}

int parse_signal(char *signal) {
    char *token;
    char *delim = ",";
    token = strtok(signal, delim);
    if (token == NULL) {
        fprintf(stderr, "Error parse order id\n");
        return -1;
    }
    char signal_t = token[0];

    if (signal_t == 'C') {
        token = strtok(NULL, delim);
        if (token == NULL) {
            fprintf(stderr, "Error parse order id\n");
            return -1;
        }
        uint id = atoi(token);
        cancel_order(id);
    } else if (signal_t == 'O') {
        token = strtok(NULL, delim);
        if (token == NULL) {
            fprintf(stderr, "Error parse order id\n");
            return -1;
        }
        uint oid = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            fprintf(stderr, "Error parse order side [%c]\n", token[0]);
            return -1;
        }
        char side = token[0];
        if (side != 'S' && side != 'B') {
            fprintf(stderr, "Error parse order side [%c]\n", side);
            return -1;
        }

        token = strtok(NULL, delim);
        if (token == NULL) {
            fprintf(stderr, "Error parse order quanity\n");
            return -1;
        }
        uint qty = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            fprintf(stderr, "Error parse order price\n");
            return -1;
        }
        double price = atof(token);

        Order *new_order = (Order *)malloc(sizeof(Order));
        new_order->oid = oid;
        new_order->side = side;
        new_order->qty = qty;
        new_order->price = price;

        process_order(new_order); // обрабатываем сигнал
    } else {
        fprintf(stderr, "Wrong signal \"%c\" received!\n", signal_t);
        return -1;
    }

    return 0;
}

int process_order(Order *new_order) {
    queue_t *q_get = new_order->side == 'S' ? buy_q : sell_q;
    queue_t *q_put = new_order->side == 'S' ? sell_q : buy_q;
    void * cmp = new_order->side == 'S' ? &cmp_buy_orders : &cmp_sell_orders;
    Order *order = (Order *)malloc(sizeof(Order));
    while (1) {
        // берем из отсортированного списка верхушку
        queue_sort(q_get, (int (*)(void *, void *))cmp);
        int retval = queue_get(q_get, (void **)&order);
        // если есть заявки
        if (retval == 0) {
            // если цена не удовлетворяет правилам покупки/продажи
            if ((new_order->side == 'B' && new_order->price < order->price) ||
                (new_order->side == 'S' && new_order->price > order->price)) {
                // возвращаем заявку обратно откуда взяли
                queue_put(q_get, order);
                // добавялем новую заявку в пул
                queue_put(q_put, new_order);
                break;
            }
            // если товара больше чем хотят
            if (new_order->qty >= order->qty) {
                // уменьшаем количество доступного товара
                new_order->qty -= order->qty;
                // формируем отчет о совершенной сделке
                make_trade_receipt(order->side, new_order->oid, order->oid,
                                   order->qty, order->price);
                // удаляем выполненную заявку
                free(order);
                // если обе заявки полностью удовлетворены выходим
                if (new_order->qty == 0) {
                    free(new_order);
                    break;
                }
                continue; // иначе берем следующую заявку
            } else { // если товара меньше чем хотят
                // уменьшаем количество доступного товара
                order->qty -= new_order->qty;
                // формируем отчет о совершенной сделке
                make_trade_receipt(order->side, new_order->oid, order->oid,
                                   new_order->qty, order->price);
                // возвращаем заявку с меньшим кол-вом обратно откуда взяли
                queue_put(q_get, order);
                free(new_order);
                break;
            }
        } else if (retval == Q_ERR_NUM_ELEMENTS) { // нет заявок в очереди
            // добавялем новую заявку в соответствующий пул
            queue_put(q_put, new_order);
            free(order);
            break;
        } else {
            fprintf(stderr, "Error while getting from queue! %d\n", retval);
            free(order);
            free(new_order);
            return -1;
        }
    }

    return 0;
}

int cancel_order(int oid) {
    Order *order = (Order *)malloc(sizeof(Order));
    int retval = -1;

    retval =
        queue_get_filtered(sell_q, (void **)&order,
                           (int (*)(void *, void *))cmp_filter, (void *)&oid);
    if (retval != 0) {
        retval = queue_get_filtered(buy_q, (void **)&order,
                                    (int (*)(void *, void *))cmp_filter,
                                    (void *)&oid);
        if (retval != 0) {
            //fprintf(stderr, "Can't find order with id=%d\n", oid);
            free(order);
            return -1;
        }
    }

    Cancel *receipt = (Cancel *)malloc(sizeof(Cancel));
    receipt->oid = oid;

    queue_put(cancel_q, receipt);

    printf("X,%d\n", order->oid);

    free(order);
    return 0;
}

int make_trade_receipt(char side, uint id_initiator, uint id_consumer, int qty,
                       double price) {
    Trade *receipt = (Trade *)malloc(sizeof(Trade));

    static int trade_id;

    receipt->side = side;
    receipt->id = ++trade_id;
    receipt->oid1 = id_consumer;
    receipt->oid2 = id_initiator;
    receipt->qty = qty;
    receipt->price = price;

    queue_put(trade_q, receipt);

    printf("T,%d,%c,%d,%d,%d,%.2f\n", trade_id, side, id_consumer, id_initiator,
           qty, price);

    return 0;
}

int cmp_sell_orders(Order *a, Order *b) {
    if (a->price < b->price)
        return -1;
    else if (a->price > b->price)
        return 1;
    else if (a->oid < b->oid)
        return -1;
    else
        return 1;
}
int cmp_buy_orders(Order *a, Order *b) {
    if (a->price < b->price)
        return 1;
    else if (a->price > b->price)
        return -1;
    else if (a->oid < b->oid)
        return -1;
    else
        return 1;
}
int cmp_receipts(Trade *a, Trade *b) {
    if (a->id < b->id)
        return 1;
    else if (a->id > b->id)
        return -1;
    else
        return 0;
}
int cmp_filter(Order *a, uint *id) {
    if (a->oid < *id)
        return 1;
    else if (a->oid > *id)
        return -1;
    else
        return 0;
}

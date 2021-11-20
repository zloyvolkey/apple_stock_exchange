#ifndef STOCK_H_
#define STOCK_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "Queue/queue.h"

#define SIGNAL_BUF_SIZE 100

typedef struct {            // O,<OID>,<Side>,<Qty>,<Price> заявка на покупку
    unsigned int oid;       // <OID> - уникальный идентификатор заявки (число)
    char side;              // <Side> - покупка(B)/продажа(S)
    unsigned int qty;       // <Qty> - количество купить/продать. Целое числ
    double price;           // <Price> - цена покупки/продажи. Число с плавающей
                            // точкой – максимум 2 знака после запятой
} Order ;

typedef struct {            // C,<OID> отмена ранее выставленной завяки
    unsigned int oid;
} Cancel;

typedef struct {            // T,<ID>,<Side>,<OID1>,<OID2>,<Trade Qty>,<Trade Price>
    unsigned int id;        // <ID> - уникальный id
    char side;              // <Side> - сторона сделки. Buy or Sell
    unsigned int oid1;      // <OID1> - id заявки с которой совершили сделку
    unsigned int oid2;      // <OID2> - id заявки которая инициировала сделку.
    unsigned int qty;       // <Trade Qty> - количество купленного/проданного товара
    double price;           // <Trade Price> - цена сделки
} Trade;

typedef struct {            // X,<OID> информация, о том, что заявка была отменена
    unsigned int oid;
} Receipt;

uint g_trade_id = 1;        // Счетчик совершенных сделок

queue_t *sell_q;            // очереди заявок на продажу
queue_t *buy_q;             // очереди заявок на покупку
queue_t *trade_q;           // список совершенных сделок
queue_t *cancel_q;          // список отмененных заявок

/**
 * @brief Парсинг строки сигнала
 * 
 * @param signal 
 * @return 0 if everything worked, < 1 if error occured
 */
int parse_signal(char * signal );

/**
 * @brief Отмена заявки по заданному id
 * 
 * @return 0 if everything worked, < 1 if error occured
 */
int cancel_order(int oid);

/**
 * @brief исполнение заявки на покупку/продажу
 * 
 */
void process_order(Order * new_order);

/**
 * @brief Формирование отчета о совершенной сделке
 * 
 */
int make_trade_receipt(char side, uint id_initiator, uint id_consumer, int qty, double price);

/**
 * @brief функции сравнения при добавлении в очередь
 * 
 */
int cmp_sell_orders(Order *a, Order *b);
int cmp_buy_orders(Order *a, Order *b);
int cmp_receipts(Trade *a, Trade *b);

/**
 * @brief функция сравнения при взятии из очереди заявки с заданным id
 * 
 */
int cmp_filter(Order *a, uint *id);

#endif  // STOCK_H_

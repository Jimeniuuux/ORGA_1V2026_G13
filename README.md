# LogicCalc - Práctica 1

### Organización Computacional

### Práctica 1 - Vaciones Junio 2026

### Integrantes

* Daniela Odeth Velásquez Solís - 202503522
* Jose Carlos González López - 202505069
* Angel Daniel Ruiz Ramos - 202505232
* Melanie Jimena Pichiyá Choc - 202502288
* Yosselin Aracely Oxlaj González - 202503415
* Pablo Javier Alvarez Marroquin - 202300502
  

LogicCalc es un prototipo de calculadora digital basado en lógica combinacional, diseñado como una Unidad Aritmética Lógica (ALU) básica. La práctica fue desarrollada utilizando compuertas lógicas e integrados TTL permitidos, con el objetivo de simular el funcionamiento interno de una ALU utilizada en los procesadores. El sistema es capaz de realizar operaciones aritméticas, lógicas y comparativas utilizando operandos binarios de 4 bits, mostrando los resultados mediante displays de 7 segmentos y LEDs indicadores.

# Operaciones Implementadas

## Unidad Aritmética

* Suma
* Resta
* Multiplicación
* Potencia

## Unidad Lógica

* AND
* OR
* NAND
* XNOR

## Unidad Comparativa

* Número mayor
* Número menor
* Igualdad entre números

## Tabla de Selección de Operaciones

| C | B | A | Operación      |
| - | - | - | -------------- |
| 0 | 0 | 0 | Suma           |
| 0 | 0 | 1 | Resta          |
| 0 | 1 | 0 | Multiplicación |
| 0 | 1 | 1 | Potencia       |
| 1 | 0 | 0 | AND            |
| 1 | 0 | 1 | OR             |
| 1 | 1 | 0 | NAND           |
| 1 | 1 | 1 | XNOR           |

DELETE FROM center_capacities;
DELETE FROM reservations;
DELETE FROM transactions;

INSERT INTO center_capacities VALUES (1, 1, 5);
INSERT INTO center_capacities VALUES (1, 2, 8);
INSERT INTO center_capacities VALUES (1, 3, 10);
INSERT INTO center_capacities VALUES (1, 4, 12);

INSERT INTO center_capacities VALUES (2, 1, 6);
INSERT INTO center_capacities VALUES (2, 2, 4);
INSERT INTO center_capacities VALUES (2, 3, 20);
INSERT INTO center_capacities VALUES (2, 4, 5);

INSERT INTO center_capacities VALUES (3, 1, 15);
INSERT INTO center_capacities VALUES (3, 2, 15);
INSERT INTO center_capacities VALUES (3, 3, 5);
INSERT INTO center_capacities VALUES (3, 4, 3);
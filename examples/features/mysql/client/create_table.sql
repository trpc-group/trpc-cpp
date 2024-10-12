CREATE DATABASE IF NOT EXISTS test;

USE test;

DROP TABLE IF EXISTS users;

CREATE TABLE `users` (
 `id` int NOT NULL AUTO_INCREMENT,
 `username` varchar(50) NOT NULL,
 `email` varchar(100) DEFAULT NULL,
 `created_at` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
 `meta` blob,
 PRIMARY KEY (`id`)
);

INSERT INTO users (username, email, created_at, meta) VALUES
              ('alice', 'alice@example.com', '2024-09-08 13:16:24', NULL),
              ('bob', 'bob@abc.com', '2024-09-08 13:16:24', NULL),
              ('carol', 'carol@example.com', '2024-09-08 13:16:24', NULL),
              ('rose', NULL, '2024-09-08 13:16:53', NULL);
CREATE TABLE calls
(
	service			TEXT PRIMARY KEY,
	parentservice	TEXT,
	callchainid		BLOB,
	transactionid	BLOB,
	start			NUMBER,
	end				NUMBER
);
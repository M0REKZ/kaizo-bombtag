// Copyright (C) Benjamín Gajardo (also known as +KZ)

#include "connection.h"

void IDbConnection::FormatCreateKaizoSaves(char *aBuf, unsigned int BufferSize, bool Backup) const
{
    str_format(aBuf, BufferSize,
		"CREATE TABLE IF NOT EXISTS %s_kaizo_saves%s ("
		"  Savegame TEXT COLLATE %s NOT NULL, "
		"  Map VARCHAR(128) COLLATE %s NOT NULL, "
		"  Code VARCHAR(128) COLLATE %s NOT NULL, "
		"  Timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, "
		"  Server CHAR(4), "
		"  DDNet7 BOOL DEFAULT FALSE, "
		"  SaveId VARCHAR(36) DEFAULT NULL, "
		"  PRIMARY KEY (Map, Code)"
		")",
		GetPrefix(), Backup ? "_backup" : "",
		BinaryCollate(), BinaryCollate(), BinaryCollate());
}

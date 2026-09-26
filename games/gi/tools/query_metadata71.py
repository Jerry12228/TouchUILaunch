"""Read-only queries over the database produced by recover_metadata71.py."""

import argparse
import json
from pathlib import Path
import sqlite3


def query(path: Path, kind: str, match: str, type_id: int | None, limit: int):
    sources = {
        "methods": ("method_symbols", "type_name || '.' || name", "declaring_type"),
        "types": ("types", "namespace || '.' || name", "id"),
        "fields": ("fields", "name", "declaring_type"),
        "images": ("images", "name", None),
    }
    source, expression, type_column = sources[kind]
    clauses, args = [f"instr(lower({expression}), lower(?)) > 0"], [match]
    if type_id is not None:
        if type_column is None:
            raise ValueError("--type-id does not apply to images")
        clauses.append(f"{type_column} = ?")
        args.append(type_id)
    if not 1 <= limit <= 10000:
        raise ValueError("--limit must be between 1 and 10000")
    args.append(limit)
    connection = sqlite3.connect(path.resolve().as_uri() + "?mode=ro", uri=True)
    try:
        connection.row_factory = sqlite3.Row
        rows = connection.execute(
            f"SELECT * FROM {source} WHERE {' AND '.join(clauses)} ORDER BY id LIMIT ?", args)
        result = [dict(row) for row in rows]
        for row in result:
            if "rva" in row and row["rva"] is not None:
                row["rva"] = hex(row["rva"])
        return result
    finally:
        connection.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("database", type=Path)
    parser.add_argument("--kind", choices=("methods", "types", "fields", "images"), default="methods")
    parser.add_argument("--match", default="")
    parser.add_argument("--type-id", type=int)
    parser.add_argument("--limit", type=int, default=50)
    args = parser.parse_args()
    try:
        result = query(args.database, args.kind, args.match, args.type_id, args.limit)
    except (sqlite3.Error, ValueError) as error:
        parser.exit(1, f"Query failed: {error}\n")
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()

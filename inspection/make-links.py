"""Turn the purchase URL cells into native XLSX hyperlinks without changing formulas."""

from html import escape
from pathlib import Path
from tempfile import NamedTemporaryFile
from zipfile import ZipFile

from openpyxl import load_workbook


book = Path(__file__).resolve().parent.parent / "thebus-two-robot-parts.xlsx"
sheet_path = "xl/worksheets/sheet1.xml"
rels_path = "xl/worksheets/_rels/sheet1.xml.rels"
source = load_workbook(book, read_only=True, data_only=False)
sheet = source["Two robot purchase list"]
links = [(f"G{row}", sheet[f"G{row}"].value) for row in range(8, 51)]
assert all(isinstance(url, str) and url.startswith("https://") for _, url in links)
source.close()

with NamedTemporaryFile(dir=book.parent, suffix=".xlsx", delete=False) as temp:
    temp_path = Path(temp.name)

try:
    with ZipFile(book, "r") as original, ZipFile(temp_path, "w") as updated:
        for item in original.infolist():
            data = original.read(item.filename)
            if item.filename == sheet_path:
                xml = data.decode("utf-8")
                block = "<x:hyperlinks>" + "".join(
                    f'<x:hyperlink ref="{cell}" r:id="Link{index}" '
                    'xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" />'
                    for index, (cell, _) in enumerate(links, 1)
                ) + "</x:hyperlinks>"
                assert "<x:pageMargins" in xml and "<x:hyperlinks>" not in xml
                xml = xml.replace("<x:pageMargins", block + "<x:pageMargins", 1)
                data = xml.encode("utf-8")
            elif item.filename == rels_path:
                xml = data.decode("utf-8")
                block = "".join(
                    '<Relationship Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/hyperlink" '
                    f'Target="{escape(url, quote=True)}" TargetMode="External" Id="Link{index}" />'
                    for index, (_, url) in enumerate(links, 1)
                )
                assert "</Relationships>" in xml
                xml = xml.replace("</Relationships>", block + "</Relationships>", 1)
                data = xml.encode("utf-8")
            updated.writestr(item, data)
    temp_path.replace(book)
finally:
    temp_path.unlink(missing_ok=True)

check = load_workbook(book, read_only=False, data_only=False)
assert all(check["Two robot purchase list"][cell].hyperlink.target == url for cell, url in links)
assert check["Two robot purchase list"]["F5"].value.startswith("=SUM(")
check.close()
print(f"Verified {len(links)} clickable purchase links and subtotal formula")

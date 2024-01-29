
import os
from src import code_dom
from src import c_lexer


def main():
    selfdir = os.path.dirname(__file__)

    with open(selfdir + '/../imgui/imgui.h', "r") as f:
        file_content = f.read()

    stream = c_lexer.tokenize(file_content)
    context = code_dom.ParseContext()
    # dom_root = code_dom.DOMHeaderFileSet()
    main_src_root = code_dom.DOMHeaderFile.parse(context, stream, 'imgui.h')
    print(main_src_root.list_all_children_of_type(code_dom.DOMEnum))


if __name__ == '__main__':
    main()

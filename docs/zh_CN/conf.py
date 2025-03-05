try:
    from conf_common import *
except ImportError:
    import os
    import sys
    sys.path.insert(0, os.path.abspath('..'))
    from conf_common import *

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

language = 'zh_CN'

html_theme_options['path_to_docs'] = "docs/zh_CN"

html_theme_options['icon_links'] = [
        {
            "name": "English",
            "url": "https://docs.nineday.cc/projects/headtracker-esp32/en/latest/index.html#"
            "icon": "fa-solid fa-language",
            "type": "fontawesome",
        }
   ]


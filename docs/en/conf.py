try:
    from conf_common import *
except ImportError:
    import os
    import sys
    sys.path.insert(0, os.path.abspath('..'))
    from conf_common import *

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

language = 'en'

html_theme_options['path_to_docs'] = "docs/en"

html_theme_options['icon_links'] = [
            {
            "name": "GitHub",
            "url": "https://github.com/NineDayCC/HeadTracker_ESP32",  # required
            "icon": "fa-brands fa-github",
            "type": "fontawesome",
        },
        {
            "name": "中文",
            "url": "https://docs.nineday.cc/projects/headtracker-esp32/zh_CN/latest/index.html",
            "icon": "fa-solid fa-language",
            "type": "fontawesome",
            "attributes": {
               "target" : "_self",
            }
        }
   ]
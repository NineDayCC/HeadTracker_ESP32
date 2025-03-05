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
            # Label for this link
            "name": "GitHub",
            # URL where the link will redirect
            "url": "https://github.com/NineDayCC/HeadTracker_ESP32",  # required
            # Icon class (if "type": "fontawesome"), or path to local image (if "type": "local")
            "icon": "fa-brands fa-github",
            # The type of image to be used (see below for details)
            "type": "fontawesome",
        },
        {
            "name": "English",
            "url": "https://docs.nineday.cc/projects/headtracker-esp32/en/latest/index.html#",
            "icon": "fa-solid fa-language",
            "type": "fontawesome",
        }
   ]


{
  "targets": [
    {
      "target_name": "gt_usb",
      "sources": [ "usb_module.cc" ],
      "cflags_cc": [ "-g" ],
      "include_dirs" : ["<!(node -e \"require('nan')\")"]
    }
  ]
}

#include "resolution_preset.h"

namespace xtcore {
namespace render {

// https://en.wikipedia.org/wiki/Graphics_display_resolution
static const resolution_preset_t k_resolution_presets[] = {
      {  128,  768, "VStrip"   } // Cubemaps
    , {  512, 3072, "VStrip"   }
    , { 1024, 6144, "VStrip"   }
    , {  768,  128, "HStrip"   }
    , { 3072,  512, "HStrip"   }
    , { 6144, 1024, "HStrip"   }
    , {  500,  500, "Square"   } // Square
    , {  800,  800, "Square"   }
    , { 1024, 1024, "Square"   }
    , { 2048, 2048, "Square"   }
    , { 4096, 4096, "Square"   }
    , {  640,  360, "nHD"      } // High Definition
    , {  960,  540, "qHD"      }
    , { 1280,  720, "HD"       }
    , { 1600,  900, "HD+"      }
    , { 1920, 1080, "FHD"      }
    , { 2160, 1440, "FHD+"     }
    , { 2048, 1080, "DCI 2K"   }
    , { 2560, 1440, "QHD/WQHD" }
    , { 3200, 1800, "QHD+"     }
    , { 3440, 1440, "UWQHD"    }
    , { 3840, 1600, "UW4K"     }
    , { 3840, 2160, "4K UHD"   }
    , { 4096, 2160, "DCI 4K"   }
    , { 5120, 2160, "UW5K"     }
    , { 5120, 2880, "5K UHD+"  }
    , { 7680, 3200, "UW8K"     }
    , { 7680, 4320, "8K UHD"   }
    , {  160,  120, "QQVGA"    } // Video Graphics Array
    , {  240,  160, "HQVGA"    }
    , {  320,  240, "QVGA"     }
    , {  400,  240, "WQVGA"    }
    , {  480,  320, "HVGA"     }
    , {  640,  480, "VGA/SD"   }
    , {  768,  480, "WVGA"     }
    , {  854,  480, "FWVGA"    }
    , {  800,  600, "SVGA"     }
    , {  960,  640, "DVGA"     }
    , { 1024,  576, "WSVGA 576"}
    , { 1024,  600, "WSVGA 600"}
    , { 1024,  768, "XGA"      } // Extended Graphics Array
    , { 1366,  768, "WXGA"     }
    , { 1152,  864, "XGA+"     }
    , { 1440,  900, "WXGA+"    }
    , { 1280, 1024, "SXGA"     }
    , { 1400, 1050, "SXGA+"    }
    , { 1680, 1050, "WSXGA+"   }
    , { 1600, 1200, "UXGA"     }
    , { 1920, 1200, "WUXGA"    }
    , { 2048, 1152, "QWXGA"    } // Quad Extended Graphics Array
    , { 2048, 1536, "QXGA"     }
    , { 2560, 1600, "WQXGA"    }
    , { 2560, 2048, "QSXGA"    }
    , { 3200, 2048, "WQSXGA"   }
    , { 3200, 2400, "QUXGA"    }
    , { 3840, 2400, "WQUXGA"   }
    , { 4096, 3072, "HXGA"     } // Hyper Extended Graphics Array
    , { 5120, 3200, "WHXGA"    }
    , { 5120, 4096, "HSXGA"    }
    , { 6400, 4096, "WHSXGA"   }
    , { 6400, 4800, "HUXGA"    }
    , { 7680, 4800, "WHUXGA"   }
};

const resolution_preset_t *resolution_presets(size_t &count)
{
    count = sizeof(k_resolution_presets) / sizeof(k_resolution_presets[0]);
    return k_resolution_presets;
}

} /* namespace render */
} /* namespace xtcore */

# Get started with falcon

This guide will teach everything you need to know in order to get started building fast & lightweight http applications with falcon.

## Integration

The easiest way to integrate falcon into your cpp application is by including the [fc.h](https://) header file into your application and then link your executable with the [libfc.a](https://...) static library.

#### Example

```cpp
#include "fc.h"

int main() {
  fc::app app;
  app.get("/", [](fc::request req) { return fc::response::ok(); });
  app.listen(":8080");
}
```

Now compile and link with the `libfc`

```shell
g++ main.cpp -libfc -l./
```

### CMake

To integrate falcon into a cmake project you need

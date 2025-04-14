# Falcon Http (experimental)

Falcon is http library for `cpp` that handles multiple concurrency connections asynchronously on single thread allowing users to write fast & lightweight http applications

### Example

```cpp
#include "fc.h"
#include <iostream>

fc::response users_find(fc::request req) {/*...*/}
// other handlers and middlewares...

int main(int argc, char *argv[]) {
  fc::app app;
  fc::router router("/users");

  // Middlewares
  router.use(auth_middleware);
  // some kind of 'logging' middleware :)
  router.use([](fc::request req) {
    std::cout << "received a request at users/" << std::endl;
    return req.next();
  });

  // Routes
  router.get("", users_find);
  router.get("/:id", users_find_id);
  router.post("", users_create);
  router.delet("/:id", users_delete);

  app.use(router);
  app.listen(":8000");
}
```

## Get Started

For full start guide please refer to this [link](https://)

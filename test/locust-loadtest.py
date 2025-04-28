from locust import HttpUser, between, task


class WebsiteUser(HttpUser):
    wait_time = between(5, 15)

    def on_start(self):
        pass

    @task
    def index(self):
        self.client.get("/")

    @task
    def about(self):
        self.client.get("/users")

    @task
    def x(self):
        self.client.get("/users/1")

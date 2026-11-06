# PostgreSQL integration tests

Enable these tests explicitly alongside `SLONYARA_BUILD_TESTS`:

```sh
cmake -S . -B <build-dir> -DSLONYARA_BUILD_TESTS=ON -DSLONYARA_BUILD_INTEGRATION_TESTS=ON
cmake --build <build-dir>
```

Use a disposable PostgreSQL instance with password authentication (`scram-sha-256`),
not `trust`. The test account must be able to create roles and databases; a
superuser in a dedicated test instance is suitable. Tests create a unique role
and database and remove them at the end. They never use application settings or
Keychain. A forcibly interrupted test may leave its generated role/database.

```sh
export SLONYARA_TEST_PG_HOST=127.0.0.1
export SLONYARA_TEST_PG_PORT=5432
export SLONYARA_TEST_PG_USER=postgres
export SLONYARA_TEST_PG_PASSWORD=local-test-password
export SLONYARA_TEST_PG_DATABASE=postgres
ctest --test-dir <build-dir> --output-on-failure -L integration
```

Missing or invalid DB configuration fails the suite instead of silently skipping
it. Worker and ConnectionTester tests cover special characters in connection parameters, wrong passwords,
missing databases, connection refusal, cancellation, timeout, and connection
cleanup. The timeout test intentionally takes about 10 seconds.

For GitHub Actions, use an Ubuntu runner with a PostgreSQL service container,
a health check, and the same environment variables. Set `POSTGRES_PASSWORD` on
the service to match `SLONYARA_TEST_PG_PASSWORD`. Qt 6.9+ and the project's other
build dependencies are required. Run all project tests with `-L slonyara`, or
only fast tests with `-L slonyara -LE integration`. PostgreSQL integration tests
are off by default. The workflow in `.github/workflows/ci.yml` runs all suites on Linux with a
PostgreSQL 16 service, and the fast suites on macOS and Windows (MinGW UCRT64).

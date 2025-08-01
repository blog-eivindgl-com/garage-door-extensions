# Use the official .NET 9.0 runtime as base image
FROM mcr.microsoft.com/dotnet/runtime:9.0 AS base
WORKDIR /app

# Use the .NET 9.0 SDK for building
FROM mcr.microsoft.com/dotnet/sdk:9.0 AS build
ARG BUILD_CONFIGURATION=Release
WORKDIR /src

# Copy project files and restore dependencies
COPY ["garage-door-extensions-service/garage-door-extensions-service.csproj", "garage-door-extensions-service/"]
COPY ["garage-door-extensions-backend-core/garage-door-extensions-backend-core.csproj", "garage-door-extensions-backend-core/"]

# Restore dependencies
RUN dotnet restore "garage-door-extensions-service/garage-door-extensions-service.csproj"

# Copy all source files
COPY garage-door-extensions-service/ garage-door-extensions-service/
COPY garage-door-extensions-backend-core/ garage-door-extensions-backend-core/

# Build the application
WORKDIR "/src/garage-door-extensions-service"
RUN dotnet build "garage-door-extensions-service.csproj" -c $BUILD_CONFIGURATION -o /app/build

# Publish the application
FROM build AS publish
ARG BUILD_CONFIGURATION=Release
RUN dotnet publish "garage-door-extensions-service.csproj" -c $BUILD_CONFIGURATION -o /app/publish /p:UseAppHost=false

# Final stage - create the runtime image
FROM base AS final
WORKDIR /app
COPY --from=publish /app/publish .

# Create a non-root user for security
RUN adduser --disabled-password --home /app --gecos '' appuser && chown -R appuser /app
USER appuser

ENTRYPOINT ["dotnet", "garage-door-extensions-service.dll"]

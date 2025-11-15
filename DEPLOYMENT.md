# Deploying to Codocet

## Prerequisites
- GitHub account
- Codocet account (sign up at codocet.io)
- Your code pushed to GitHub

## Step-by-Step Deployment

### 1. Push to GitHub
```bash
git add .
git commit -m "Add Docker deployment support"
git push origin master
```

### 2. Deploy on Codocet

1. **Sign in to Codocet** (https://codocet.io)

2. **Create New Project**
   - Click "New Project" or "Deploy"
   - Select "Deploy from GitHub"

3. **Connect GitHub Repository**
   - Authorize Codocet to access your GitHub
   - Select repository: `fayzan101/DS-Project`
   - Branch: `master`

4. **Configure Deployment**
   - **Build Method**: Docker
   - **Dockerfile Path**: `./Dockerfile`
   - **Port**: `8080`
   - **Root Directory**: `/` (or path to HuffmanCompressor if not root)

5. **Environment Variables** (if needed)
   - None required for basic setup

6. **Deploy**
   - Click "Deploy" button
   - Wait for build to complete (2-5 minutes)

### 3. Access Your API

Once deployed, Codocet will provide:
- **Public URL**: `https://your-app-name.codocet.app`
- **API Endpoints**: Use this URL as base for all API calls

Example:
```
POST https://your-app-name.codocet.app/api/compress
GET  https://your-app-name.codocet.app/api/list
```

### 4. Test Your Deployment

Use Postman or curl:
```bash
curl https://your-app-name.codocet.app/
```

## Alternative: Render.com (Easier Alternative)

1. **Sign in to Render** (https://render.com)
2. **New Web Service** → Connect GitHub repo
3. **Settings**:
   - Environment: Docker
   - Dockerfile path: `./Dockerfile`
   - Port: 8080
4. **Deploy**

Free tier includes:
- HTTPS automatic
- Custom domain support
- Auto-redeploy on git push

## Local Docker Testing

Before deploying, test locally:

```bash
# Build image
docker build -t huffman-api .

# Run container
docker run -p 8080:8080 huffman-api

# Test
curl http://localhost:8080/
```

## Troubleshooting

**Build fails?**
- Check Dockerfile syntax
- Ensure all source files are committed
- Check build logs in Codocet dashboard

**Server won't start?**
- Verify port 8080 is exposed
- Check that directories (uploads, compressed, decompressed) exist
- Review application logs

**API not responding?**
- Ensure health check endpoint exists
- Check firewall/network settings
- Verify correct port mapping

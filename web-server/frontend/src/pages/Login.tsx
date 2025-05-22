import React, { useState } from 'react';
import {
  Box,
  Button,
  Avatar,
  Typography,
  TextField,
  IconButton,
  InputAdornment,
  Alert,
  CircularProgress,
  Container,
  Grid,
  Snackbar,
  FormControlLabel,
  Checkbox
} from '@mui/material';
import { Visibility, VisibilityOff, LockOutlined } from '@mui/icons-material';
import { useNavigate } from 'react-router-dom';

const Login: React.FC = () => {
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [rememberMe, setRememberMe] = useState(false);
  const [showPassword, setShowPassword] = useState(false);
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(false);
  const [openSnackbar, setOpenSnackbar] = useState(false);

  const navigate = useNavigate();

  const isValid = email.includes('@') && password.length >= 4;

  const handleLogin = async () => {
    setLoading(true);
    setError('');

    setTimeout(() => {
      if (email === 'admin@example.com' && password === 'admin') {
        localStorage.setItem('token', 'fake-jwt-token');
        if (rememberMe) {
          localStorage.setItem('rememberMe', 'true');
        }
        setOpenSnackbar(true);
        navigate('/dashboard');
      } else {
        setError('Email sau parolă incorectă');
      }
      setLoading(false);
    }, 1000);
  };

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (isValid) handleLogin();
    else setError('Introduceți un email valid și o parolă de minim 4 caractere.');
  };

  return (
    <Container component="main" maxWidth="xs">
      <Box
        sx={{
          marginTop: 10,
          display: 'flex',
          flexDirection: 'column',
          alignItems: 'center',
          boxShadow: 3,
          padding: 4,
          borderRadius: 2,
          backgroundColor: '#fff'
        }}
      >
        <Avatar sx={{ m: 1, bgcolor: 'primary.main' }}>
          <LockOutlined />
        </Avatar>
        <Typography component="h1" variant="h5">
          Autentificare
        </Typography>

        <Box component="form" onSubmit={handleSubmit} sx={{ mt: 3, width: '100%' }}>
          {error && <Alert severity="error" sx={{ mb: 2 }}>{error}</Alert>}

          <TextField
            label="Email"
            fullWidth
            margin="normal"
            autoComplete="email"
            autoFocus
            value={email}
            onChange={(e) => setEmail(e.target.value)}
          />

          <TextField
            label="Parolă"
            fullWidth
            margin="normal"
            type={showPassword ? 'text' : 'password'}
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            InputProps={{
              endAdornment: (
                <InputAdornment position="end">
                  <IconButton
                    aria-label="Afișează/Ascunde parola"
                    onClick={() => setShowPassword((prev) => !prev)}
                    edge="end"
                  >
                    {showPassword ? <VisibilityOff /> : <Visibility />}
                  </IconButton>
                </InputAdornment>
              )
            }}
          />
        <Grid container justifyContent="space-between" alignItems="center" sx={{ mt: 1 }}>
            <Grid item={true} {...({} as any)}>
                <FormControlLabel
                control={
                    <Checkbox
                    checked={rememberMe}
                    onChange={(e) => setRememberMe(e.target.checked)}
                    color="primary"
                    />
                }
                label="Ține-mă minte"
                />
            </Grid>
            <Grid item={true} {...({} as any)}>
                <Button onClick={() => navigate('/register')} size="small">
                Creează cont
                </Button>
            </Grid>
        </Grid>

          <Button
            type="submit"
            fullWidth
            variant="contained"
            color="primary"
            sx={{ mt: 3, mb: 2 }}
            disabled={loading}
          >
            {loading ? <CircularProgress size={24} color="inherit" /> : 'Login'}
          </Button>

            <Grid container justifyContent="flex-end">
            <Grid item={true} {...({} as any)}>
                <Typography variant="body2" color="text.secondary">
                Test: admin@example.com / admin
                </Typography>
            </Grid>
            </Grid>
        </Box>
      </Box>

      <Snackbar
        open={openSnackbar}
        autoHideDuration={3000}
        onClose={() => setOpenSnackbar(false)}
        message="Autentificare reușită!"
      />
    </Container>
  );
};

export default Login;
